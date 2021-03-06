from pathlib import Path

import luigi
from exasol_integration_test_docker_environment.lib.base.flavor_task import FlavorBaseTask
from exasol_integration_test_docker_environment.lib.base.frozendict_to_dict import FrozenDictToDict
from exasol_integration_test_docker_environment.lib.base.json_pickle_target import JsonPickleTarget
from exasol_integration_test_docker_environment.lib.base.still_running_logger import StillRunningLogger, \
    StillRunningLoggerThread
from exasol_integration_test_docker_environment.lib.config.log_config import log_config, WriteLogFilesToConsole
from exasol_integration_test_docker_environment.lib.data.database_credentials import DatabaseCredentialsParameter

from exaslct_src.exaslct.lib.tasks.test.run_db_test_result import RunDBTestResult
from exaslct_src.exaslct.lib.tasks.test.run_db_tests_parameter import RunDBTestParameter


class RunDBTest(FlavorBaseTask,
                RunDBTestParameter,
                DatabaseCredentialsParameter):
    test_file = luigi.Parameter()

    def extend_output_path(self):
        test_file_name = Path(self.test_file).name
        extension = []
        if self.language is not None:
            extension.append(self.language)
        extension.append(test_file_name)
        return self.caller_output_path + tuple(extension)

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._test_container_info = self.test_environment_info.test_container_info
        self._database_info = self.test_environment_info.database_info

    def run_task(self):
        self.logger.info("Running db tests")
        test_container = self._client.containers.get(self._test_container_info.container_name)
        bash_cmd = self.generate_test_command()
        environment, exit_code, output = self.run_test_command(bash_cmd, test_container)
        self.handle_test_result(bash_cmd, environment, exit_code, output)

    def handle_test_result(self, bash_cmd, environment, exit_code, output):
        test_output = "command: " + bash_cmd + "\n" + \
                      "environment: " + str(environment) + "\n" + \
                      output.decode("utf-8")
        is_test_ok = (exit_code == 0)
        if log_config().write_log_files_to_console == WriteLogFilesToConsole.all:
            self.logger.info("Test results for db tests\n%s"
                             % (test_output))
        if log_config().write_log_files_to_console == WriteLogFilesToConsole.only_error and not is_test_ok:
            self.logger.error("db tests failed\nTest results:\n%s"
                              % (test_output))
        test_output_file = self.get_log_path().joinpath("test_output")
        with test_output_file.open("w") as file:
            file.write(test_output)
        result = RunDBTestResult(
            test_file=self.test_file,
            language=self.language,
            is_test_ok=is_test_ok,
            test_output_file=test_output_file)
        JsonPickleTarget(self.get_output_path().joinpath("test_result.json")).write(result, 4)
        self.return_object(result)

    def run_test_command(self, bash_cmd, test_container):
        still_running_logger = StillRunningLogger(self.logger, "db tests")
        thread = StillRunningLoggerThread(still_running_logger)
        thread.start()
        environment = FrozenDictToDict().convert(self.test_environment_vars)
        environment["TEST_ENVIRONMENT_TYPE"] = self.test_environment_info.type.name
        environment["TEST_ENVIRONMENT_NAME"] = self.test_environment_info.name
        environment["TEST_DOCKER_NETWORK_NAME"] = self.test_environment_info.network_info.network_name
        if self.test_environment_info.database_info.container_info is not None:
            environment[
                "TEST_DOCKER_DB_CONTAINER_NAME"] = self.test_environment_info.database_info.container_info.container_name

        exit_code, output = test_container.exec_run(cmd=bash_cmd,
                                                    environment=environment)
        thread.stop()
        thread.join()
        return environment, exit_code, output

    def generate_test_command(self):
        credentials = f"--user '{self.db_user}' --password '{self.db_password}'"
        log_level = f"--loglevel={self.test_log_level}"
        server = f"--server '{self._database_info.host}:{self._database_info.db_port}'"
        environment = "--driver=/downloads/ODBC/lib/linux/x86_64/libexaodbc-uo2214lv2.so  " \
                      "--jdbc-path /downloads/JDBC/exajdbc.jar"
        language_definition = f"--script-languages '{self.language_definition}'"
        language = ""
        if self.language is not None:
            language = "--lang %s" % self.language
        test_restrictions = " ".join(self.test_restrictions)
        test_file = f'"{self.test_file}"'
        args = " ".join([test_file,
                         server,
                         credentials,
                         language_definition,
                         log_level,
                         environment,
                         language,
                         test_restrictions])
        cmd = f'cd /tests/test/; python -tt {args}'
        bash_cmd = f"""bash -c "{cmd}" """
        return bash_cmd
