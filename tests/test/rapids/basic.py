#!/usr/bin/env python2.7

import os
import sys

sys.path.append(os.path.realpath(__file__ + '/../../../lib'))

import udf
from udf import useData, expectedFailure
from exatest.testcase import skip

class RapidsBasics(udf.TestCase):
    def setUp(self):
        self.query('create schema rapids_basic', ignore_errors=True)

    def test_import_rapids(self):
        self.query(udf.fixindent('''
                CREATE OR REPLACE python3 scalar SCRIPT rapids_basic.import_rapids()
                returns varchar(1000) as
                
                import pandas as pd
                import numpy as np
                import cudf

                def run(ctxi):

                    pandas_df = pd.DataFrame({'a': np.random.randint(0, 100000000, size=100000000),
                                              'b': np.random.randint(0, 100000000, size=100000000)})
                                          
                    cudf_df = cudf.DataFrame.from_pandas(pandas_df)
                    pass
                /
                '''))

        row = self.query("select rapids_basic.import_rapids()")[0]


if __name__ == '__main__':
    udf.main()
