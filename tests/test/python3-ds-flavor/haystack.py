#!/usr/bin/env python2.7

import os
import sys

sys.path.append(os.path.realpath(__file__ + '/../../../lib'))

import udf
from udf import useData, expectedFailure
from exatest.testcase import skip

class HaystackBasics(udf.TestCase):
    def setUp(self):
        self.query('create schema haystackbasics', ignore_errors=True)

    def test_run_haystack(self):
        self.query(udf.fixindent('''
                CREATE OR REPLACE python3 scalar SCRIPT haystackbasics.run_haystack()
                returns varchar(2000000) as


                def run(ctx):
                    try:
                        from haystack.reader.farm import FARMReader
                        from haystack.database.base import Document
                        import os
                        os.chdir("/tmp")
                        reader = FARMReader(model_name_or_path="deepset/roberta-base-squad2", use_gpu=False, num_processes=0)
                        docs = [Document(id=1, text="My Name is Carla and I live in Berlin")]
                        pred = reader.predict(question="Who lives in Berlin?", documents=docs)
                        return str(pred)
                    except EnvironmentError as e:
                        import traceback
                        return traceback.format_exc()
                /
                '''))

        row = self.query("select haystackbasics.run_haystack()")
        print(row)


if __name__ == '__main__':
    udf.main()
