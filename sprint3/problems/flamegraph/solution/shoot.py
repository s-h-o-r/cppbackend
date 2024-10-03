import argparse
import subprocess
import time
import random
import shlex

RANDOM_LIMIT = 1000
SEED = 123456789
random.seed(SEED)

AMMUNITION = [
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1


def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server


def run(command, output=None):
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.DEVNULL)
    return process


def stop(process, wait=False):
    if process.poll() is None and wait:
        process.wait()
    process.terminate()


def shoot(ammo):
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')


def start_perf_record(pid):
    perf_proc = run('perf record -p ' + pid + ' -o perf.data')
    stop(perf_proc, wait=True)


def start_flamegraf(stack_collapse-perf='./FlameGraph/stackcollapse-perf.pl', flamegraf_path='./FlameGraph/flamegraph.pl', file_out_name='graph.svg'):
    flamegraph_proc = run('sudo perf script | ' + stack_collapse + ' | ' + flamegraf_path + ' > ' + file_out_name)
    stop(perf_proc, wait=True)


server = run(start_server())
start_perf_record(server.pid())
make_shots()
stop(server)

start_flamegraf()

time.sleep(1)
print('Job done')
