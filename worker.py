import redis
from rq import Worker, Connection

listen = ['default']
REDIS_HOST = "192.168.64.107"
REDIS_PORT = '6379' 
redis_url = f'redis://{REDIS_HOST}:{REDIS_PORT}/0'
conn = redis.from_url(redis_url)

if __name__ == '__main__':
    with Connection(conn):
        worker = Worker(listen)
        worker.work()