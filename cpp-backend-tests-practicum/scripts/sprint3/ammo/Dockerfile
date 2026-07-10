from praktikumcpp/practicum_cpp_backend_server:latest

COPY solution /solution
COPY test_s03_ammo.py /test_s03_ammo.py
WORKDIR /solution
RUN echo "yandex-tank -c /solution/load.yaml /solution/ammo.txt" >> run.sh
RUN echo "pytest --junitxml=/ammo.xml /test_s03_ammo.py" >> run.sh
RUN chmod +x run.sh
CMD ./run.sh
