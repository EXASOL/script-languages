from typing import Dict

import luigi

from exaslct_src.exaslct.lib.tasks.build.docker_flavor_build_base import DockerFlavorBuildBase
from exaslct_src.test_environment.src.lib.base.flavor_task import FlavorsBaseTask
from exaslct_src.test_environment.src.lib.docker.images.push.docker_push_parameter import DockerPushParameter
from exaslct_src.test_environment.src.lib.docker.images.push.push_task_creator_for_build_tasks import \
    PushTaskCreatorFromBuildTasks


class DockerFlavorsPush(FlavorsBaseTask, DockerPushParameter):
    goals = luigi.ListParameter()

    def register_required(self):
        tasks = self.create_tasks_for_flavors_with_common_params(
            DockerFlavorPush)  # type: Dict[str,DockerFlavorPush]
        self.image_info_futures = self.register_dependencies(tasks)

    def run_task(self):
        image_infos = self.get_values_from_futures(self.image_info_futures)
        self.return_object(image_infos)


class DockerFlavorPush(DockerFlavorBuildBase, DockerPushParameter):
    goals = luigi.ListParameter()

    def get_goals(self):
        return self.goals

    def run_task(self):
        build_tasks = self.create_build_tasks(shortcut_build=not self.push_all)
        push_task_creator = PushTaskCreatorFromBuildTasks(self)
        push_tasks = push_task_creator.create_tasks_for_build_tasks(build_tasks)
        image_infos = yield from self.run_dependencies(push_tasks)
        self.return_object(image_infos)