### Purpose
This project has a frontend that allows a user to enter python code in a textblock, which is then sent to the backend to run. 
Docker is used to isolate the environment the code runs in, to avoid the numerous security issues that arise from running random code on a server.

### Running
The code can be run by using `docker-compose up`. This will host the project on localhost, exposing ports 80 for frontend, and 8080 for backend. 
