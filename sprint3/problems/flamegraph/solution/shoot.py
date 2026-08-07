import argparse
import signal
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
    if process.poll() is None:
        if wait:
            process.wait()
        else:
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


if __name__ == '__main__':
    server_cmd = start_server()
    server = run(server_cmd)
    time.sleep(0.5)

    perf = run(f'perf record -g -p {server.pid} -o perf.data')
    time.sleep(0.5)

    make_shots()

    perf.send_signal(signal.SIGINT)
    perf.wait()

    with open('graph.svg', 'w') as out_file:
        p1 = subprocess.Popen(['perf', 'script', '-i', 'perf.data'], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        p2 = subprocess.Popen(['FlameGraph/stackcollapse-perf.pl'], stdin=p1.stdout, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        p1.stdout.close()
        p3 = subprocess.Popen(['FlameGraph/flamegraph.pl'], stdin=p2.stdout, stdout=out_file, stderr=subprocess.DEVNULL)
        p2.stdout.close()
        p3.communicate()

    stop(server)
    time.sleep(1)
    print('Job done')