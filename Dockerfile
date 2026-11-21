# One-command PSP build.  Nothing to install locally.
#
#   docker build -t lostrune .
#   docker run --rm -v "$PWD:/src" lostrune
#
# Produces Adventure.iso in the project folder.
FROM pspdev/pspdev:latest

WORKDIR /src
COPY . /src

RUN make && ./build_iso.sh

CMD ["/bin/sh", "-c", "make && ./build_iso.sh"]
