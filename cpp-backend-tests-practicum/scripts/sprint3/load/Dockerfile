from praktikumcpp/practicum_cpp_backend_server:latest

COPY solution /solution
COPY test_s03_load.py /test_s03_load.py
WORKDIR /solution
run echo "yandex-tank -c /solution/load.yaml /solution/ammo.txt" >> run.sh
run echo "pytest --junitxml=/load.xml /test_s03_load.py" >> run.sh
RUN chmod +x run.sh
CMD ./run.sh
