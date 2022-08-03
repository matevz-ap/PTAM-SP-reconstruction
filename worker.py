import os

import redis
from rq import Worker, Connection

listen = ['default']
conn = redis.from_url("redis://redis:6379/0")

if __name__ == '__main__':
    with Connection(conn):
        worker = Worker(listen)
        worker.work()