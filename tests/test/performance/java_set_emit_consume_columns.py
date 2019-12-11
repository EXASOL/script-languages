#!/usr/bin/env python2.7
# encoding: utf8

import os
import sys
import time

sys.path.append(os.path.realpath(__file__ + '/../../../lib'))
sys.path.append(os.path.realpath(__file__ + '/..'))

import udf
from abstract_performance_test import AbstractPerformanceTest


class SetEmitConsumeNextOnlyJavaPerformanceTest(AbstractPerformanceTest):

    def setUp(self):
        self.create_schema()
        self.generate_data_linear(500)
        self.query(udf.fixindent('''
                CREATE JAVA SET SCRIPT CONSUME_NEXT(
                    intVal DECIMAL(9,0), 
                    longVal DECIMAL(18,0), 
                    bigdecimalVal DECIMAL(36,0), 
                    decimalVal DECIMAL(9,2),
                    doubleVal DOUBLE, 
                    doubleIntVal DOUBLE, 
                    stringVal VARCHAR(100), 
                    booleanVal BOOLEAN, 
                    dateVal DATE, 
                    timestampVal TIMESTAMP) EMITS (count_value INT) AS
                import java.math.BigDecimal;
                class CONSUME_NEXT {
                    static void run(ExaMetadata exa, ExaIterator ctx) throws Exception {
                        int count=0;
                        while(ctx.next()){
                            Integer intVal = ctx.getInteger(0);
                            Long longVal = ctx.getLong(1);
                            BigDecimal bigdecimalVal = ctx.getBigDecimal(2);
                            BigDecimal decimalVal = ctx.getBigDecimal(3);
                            Double doubleVal = ctx.getDouble(4);
                            Double doubleIntVal = ctx.getDouble(5);
                            String stringVal = ctx.getString(6);
                            Boolean booleanVal = ctx.getBoolean(7);
                            java.sql.Date dateVal = ctx.getDate(8);
                            java.sql.Timestamp timestampVal = ctx.getTimestamp(9);
                            count=count+1;
                        }
                    }
                }
                '''))
        self.query("commit")
    
    def tearDown(self):
        self.cleanup(self.schema)

    def test_consume_next(self):
        self.run_test(15, 3, 2.0, "SELECT CONSUME_NEXT(intVal,longVal,bigdecimalVal,decimalVal,doubleVal,doubleIntVal,stringVal,booleanVal,dateVal,timestampVal) FROM T")

if __name__ == '__main__':
    udf.main()

# vim: ts=4:sts=4:sw=4:et:fdm=indent

