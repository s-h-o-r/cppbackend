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


print(start_server())
server = run(start_server())

record = run('sudo perf record -g -p' + str(server.pid) + ' -o perf.data ')

make_shots()
stop(server)

time.sleep(1)

p1 = subprocess.Popen(shlex.split("perf script -i perf.data"), stdout=subprocess.PIPE)
p2 = subprocess.Popen(shlex.split("./FlameGraph/stackcollapse-perf.pl"), stdin=p1.stdout, stdout=subprocess.PIPE)
with open("graph.svg", "w") as f:
    p3 = subprocess.Popen(shlex.split("./FlameGraph/flamegraph.pl"), stdin=p2.stdout, stdout=f)
p3.wait()

print('Job done')
