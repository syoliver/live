pushd %~dp0

docker image build -t live .
cd ../..
docker container run -v %CD%:/develop/src -v //var/run/docker.sock:/var/run/docker.sock -p 8080:8080 -it live
popd