#!/usr/bin/python3
import logging
import logging.handlers
import os
import re
import datetime
import configparser
import time
import psycopg2 as pg
from flask import Flask, json, request
from flask_caching import Cache
from flask_log_request_id import RequestID, current_request_id

from psycopg2.extras import RealDictCursor


INVALID_PAGE_INFO = "Invalid page"
INVALID_TABLE_ERROR = "Invalid table"


class PostgresWrapper:
    def __init__(self):
        self.connection = None

    def connect(self, **kwargs):
        logging.info("Connect to DB with args: %s" % str(kwargs))
        self.connection = pg.connect(**kwargs)
        logging.info("Connected to DB successfully")

    def close(self):
        self.connection.close()
        logging.info("Disconnected from DB successfully")

    def fetch(self, cache, cursor, max_num_items, page):
        current_page = 0
        response = INVALID_PAGE_INFO
        while True:
            res = cursor.fetchmany(max_num_items)

            # Fetch MAX_ITEMS_PER_PAGE items assuming we have more
            has_more = len(res) == max_num_items

            ret = f'"page": "{current_page}", "has_more": "{has_more}", "result": {json.dumps(res)}'

            url = request.url
            if request.args.get('page'):
                # remove the trailing &page=***
                url = url[:url.rfind('&page=')]

            key = f"{url}&page={current_page}" if current_page != 0 else url

            if not cache.has(key):
                cache.set(key, ret)

            if current_page == page:
                response = ret

            current_page += 1

            if not has_more:
                break

        return response

    def get_response_page(self, cache, query, page):
        with self.connection.cursor(cursor_factory=RealDictCursor) as cursor:
            logging.debug("Executing query: %s" % query)
            cursor.execute(query)

            MAX_ITEMS_PER_PAGE = 100
            response = self.fetch(cache, cursor,
                                  MAX_ITEMS_PER_PAGE, page)

            return response

    def get_response(self, query):
        with self.connection.cursor(cursor_factory=RealDictCursor) as cursor:
            logging.debug("Executing query: %s" % query)

            cursor.execute(query)

            res = cursor.fetchall()
            return f'"result": {json.dumps(res)}'


app = Flask(__name__)
RequestID(app)


@app.before_request
def before_request_func():
    now = datetime.datetime.now()
    request.start_time = now
    logging.info(
        f"Request {current_request_id()}, url: {request.url}, worker pid: {os.getpid()}")


@app.after_request
def after_request_func(response):
    request.status_code = response.status_code
    return response


@app.teardown_request
def teardown_request_func(error):
    now = datetime.datetime.now()
    request_time = now - request.start_time
    status = request.status_code if hasattr(request, 'status_code') else 'None'
    if error:

        logging.warning(
            f"Request {current_request_id()} was aborted within {request_time.total_seconds()} seconds with status={status} with error: {error}")
    else:
        logging.info(
            f"Request {current_request_id()} was finished within {request_time.total_seconds()} seconds with status={status}")


@app.route("/sleep")
def sleep():
    pid = os.getpid()
    t = request.args.get("time", default=None, type=int)
    logging.info("Sleep for %s seconds, pid: %s" % (t, pid))
    time.sleep(t)
    logging.info("Sleep done, pid: %s" % pid)
    return "Sleep done for %s seconds, pid: %s" % (t, pid)


@app.route("/get")
def get_table():
    resp = app.cache.get(request.url)
    if resp is None:
        logging.info(
            f"Request {current_request_id()}, cache miss")

        table = request.args.get("table", default=None, type=str)
        page = request.args.get("page", default=0, type=int)
        query = f"SELECT * FROM {table};"

        try:
            resp = app.dbconn.get_response_page(
                app.cache, query, page)
        except Exception as e:
            return INVALID_TABLE_ERROR

    else:
        logging.info(
            f"Request {current_request_id()}, cache hit")

    return resp


@app.route("/")
def get_root():
    query = """SELECT relname as table_name
               FROM pg_class
        WHERE relkind = 'r' and relname !~ '^(pg_|sql_)'
    """
    resp = app.dbconn.get_response(query)
    return resp


def config_logging(app_name, cfg):
    DEFAULT_LOG_DIR = "/tmp/logs"
    DEFAULT_BACKUP_COUNT = 30
    DEFAULT_FILE_LOG_LEVEL = logging.DEBUG
    DEFAULT_STDOUT_LOG_LEVEL = logging.INFO
    DEFAULT_STDOUT_LOG_ENABLED = False

    log_dir = cfg.get("file_log_dir", DEFAULT_LOG_DIR)
    log_file = f"{log_dir}/{app_name}.log"
    file_logging = logging.handlers.TimedRotatingFileHandler(
        filename=log_file, when='midnight', backupCount=cfg.get("backup_count", DEFAULT_BACKUP_COUNT))
    log_handlers = [file_logging]

    file_logging.setLevel(
        cfg.get("file_log_level", DEFAULT_FILE_LOG_LEVEL).upper())

    stdlog_enabled = cfg.get('stdout_log', DEFAULT_STDOUT_LOG_ENABLED)
    if stdlog_enabled:
        std_log = logging.StreamHandler()
        std_log.setLevel(cfg.get("stdout_log_level",
                         DEFAULT_STDOUT_LOG_LEVEL).upper())
        log_handlers.append(std_log)

    logging.basicConfig(format='%(asctime)s %(levelname)s %(message)s (%(filename)s:%(lineno)s)', level=logging.DEBUG, datefmt='%H:%M:%S.%s',
                        handlers=log_handlers)


def config_server():
    config = configparser.ConfigParser()
    config.read('config.ini')

    with app.app_context():
        app_config = config['application']
        app_name = app_config['name']

        log_config = config['logging']
        config_logging(app_name, log_config)

        postgres_config = config['postgres']
        logging.info("Postgres config: %s" % dict(postgres_config))
        app.dbconn = PostgresWrapper()
        app.dbconn.connect(**postgres_config)

        cache_cfg = dict(((k.upper(), v) for k, v in config['cache'].items()))
        cache_cfg.setdefault('CACHE_KEY_PREFIX', app_name)
        logging.info("Cache config: %s" % cache_cfg)
        app.cache = Cache(app, True, cache_cfg)

    return app


if __name__ == "__main__":
    app = config_server()
    app.run()
else:
    gunicorn_app = config_server()
