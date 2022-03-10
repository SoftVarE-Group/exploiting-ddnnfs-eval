FROM python:3.9.10-slim

WORKDIR /benchmark

# c2d is a 32-bit binary
RUN dpkg --add-architecture i386
RUN apt-get update -y
RUN apt-get install libc6:i386 libncurses5:i386 libstdc++6:i386 libgmp-dev -y

COPY ./requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

COPY ./ ./

ENTRYPOINT ["python3", "./run.py"]
