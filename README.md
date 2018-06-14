# COMP 304 : Operating Systems
# Project : Metro Simulation with POSIX Threads

This project is about scheduling the trains that originate from four different locations called A, B, E and F and use a tunnel to reach the destination point. As trains will be created concurrently but only one train can pass the tunnel at a time, threads are used to manage train creation and work as a control center. In order to manage concurrency, POSIX mutex and semaphores are employed. Detailed description can be found in the [project description](https://github.com/Matiatus/Metro-Simulation-with-Pthreads/blob/master/Project_Description.pdf).

## Getting Started

### How to Run the Project?: 

C++ is preferred for this project. In order to run the project on MacOS, any IDE for C++, such as CLion or Xcode, can be used. To run the project in Linux environment or on Terminal of any Unix machine: 
```
g++ -o <output_file_name> main.cpp –lpthread
```
The output file can accept two paremeters. First input is the probability used for train creation, it can be in interval [0.0, 1.0] and can be provided as follows:
```
./<output_file_name> <probability>
```
Then with the simulation flag, a simulation time in seconds can be provided as well:
```
./<output_file_name> <probability> -s <seconds>
```
Las option, if there are no inputs provided, probability is set to 0.5 and simulation time is set to 60 seconds by default. 

## Design of the Project:

### Structs
In order to implement this project in a single .cpp file without writing any class, we defined three structs called **metroTrain**, **metroTime** and **metroEvent**: 
* **metroTrain** struct denotes the train in the project description and holds the length, starting point, destination point, start time and arrival time and ID of the train. 
* **metroTime** struct is consisted of hour, minute and second and is employed to hold start or end time of the train. 
* **metroEvent** struct holds the event type, event time, train ID associated with the event, queue of waiting trains (if available) and clear time if there is a starvation since there are more than 10 trains in the system.  

## General Flow

### Threads
In order to handle concurrency, there are four threads for train creation in locations A, B, E and F, and a control center thread that controls the train passing from the tunnel. Control center thread manages the scheduling of trains to pass the tunnel to reach the destination. Each thread work once in a second and waits for the next second until simulation time is completed. To create the threads, POSIX threads are used. 

When the simulation starts, four threads start to create trains with given probability, hold the start (creation) time by obtaining the current time of the simulation, decide the arrival point and length of the train by generating random numbers between 0.0 and 1.0 and compare the generated probability with the probability of being 100 m or 200 m and probability of arriving to E and F for A and B and vice versa. Each created train is added to the fresh train queue after obtaining the mutex for modifying freshTrain queue, which is named **freshtx**. 

### Control-Center Thread
Control center thread first determines if there is starvation or clearance in the system, by checking the number of total trains on rails outside the tunnel. Then, creates an event for log, according to the type of the event, and adds the event to the end of _eventQueue_. 

If the tunnel is available for passing (i.e. not occupied), assigns a train to pass the tunnel according to the priority given in the assignment. Control center follows these procedure to choose which train will pass the tunnel. To indicate whether the tunnel is available or not, there is a metroTime type of field called **tunnelOccupied** that specifies the time that the tunnel will be available. If the tunnel is available, then the second of tunnelOccupied becomes -1, which simply works as a flag showing the tunnel is available. If the tunnel is not available, control block checks if the train is broken down inside the tunnel by generating a random number between 0 and 1 and compare the number with 0.1 to label the train as _broken down_ and create an event to indicate that a train is broken down inside the tunnel at that time. If there is a break, the occupation time of the tunnel gets updated, by increasing by four seconds. 

After that, since all the trains take 1 second after creation to come to the tunnel entrances, control center goes to the freshly created metroTrain queue and moves the trains to the entrances, a.k.a _trainQueue_, if 1 second has passed after the creation of the train. In order to modify _freshTrain_ and _trainQueue_ queues, the control center thread needs to obtain **freshtx** mutex. 

### Logs

There are two kinds of logs expected from the project. First type of log demonstrates the trains created in the system. In this log, integral information regarding to trains such as train ID, starting point, end point, length, arrival time and departure time can be found. Second type of log demonstrates the events occurred in the simulation. Events can be summarized as “Tunnel Passing”, “Breakdown”, “System Overload”, “Tunnel Cleared”. For each event, the time that the event occurred is also provided. If there is a queue in the system for tunnel passing, this log also demonstrates the queue. If the event is “Tunnel Passing”, the ID of the train that passed the tunnel is provided.

Log files are created in “Main” after simulation ends. 

### Methods

There are 9 methods in the system used to complete simulation. 

_timeLessThan_ method gets two metroTime argument and compares them to indicate whether the first argument is less than the second one. 

_createTrain_ method creates a train with given train ID.

_logEverything_ method is employed after simulation ends to create the desired log files. 

_trainAssigner_ method gets a train ID as input and calls createTrain method if there is no starvation in the system. 

_calculateTimeInterval_ method calculates the end time of the simulation by adding the simulation time to starting time of the simulation. 

_controlCenter_ method completes every responsibility of control center thread explained in above section.

_createEvent_ method gets name of the event, current time and ID (-1 or train ID according to the event type) and creates the event to add to the end of the eventQueue.

_adjustTime_ method is used to update the time given as input.

_letTrainPass_ method accepts the current time and returns the next available time of the tunnel. According to the procedure explained in the project description file, a train in the trainQueue is selected to pass the tunnel. The train that passes the tunnel is added to finitoTrains queue, which contains the trains that passed the tunnel.

More detailed explanations about the code can be found in the comments. 

## Deployment

### Built With

* [Jetbrains / CLion](https://www.jetbrains.com/clion/)

### Authors
* Mert Atila Sakaoğulları
* Pınar Topçam
