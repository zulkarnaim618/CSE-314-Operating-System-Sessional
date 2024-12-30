#include<iostream>
#include<pthread.h>
#include<semaphore.h>
#include<unistd.h>
#include<chrono>
#include<cstdlib>
#include<ctime>
#include<random>
#define NOT_STARTED_YET 0
#define WAITING_TO_PRINT 1
#define NOTIFIED 3
#define FINISHED 4
#define FREE 0
#define BUSY 1
using namespace std;
pthread_mutex_t globalPrintlock;
chrono::system_clock::time_point start;
sem_t bindingStation;
int n,m,w,x,y;
const int printingStationNum = 4;
const int bindingStationNum = 2;
pthread_mutex_t entryBookLock,entryBookWriteLock;
int submissionCount = 0;
int readerCount = 0;
int stuffNum = 2;
int interArrivalRate = 5;
default_random_engine generator;
poisson_distribution<int> distribution(interArrivalRate);
class PrintingStation {
public:
    pthread_mutex_t lock;
    int state;
    PrintingStation() {
        this->state = FREE;
        pthread_mutex_init(&lock,NULL);
    }
    ~PrintingStation() {
        pthread_mutex_destroy(&lock);
    }
};
class Student {
public:
    int id;
    int state;
    pthread_t thread;
    sem_t printSem;
    Student(int id) {
        this->id = id;
        this->state = NOT_STARTED_YET;
        sem_init(&printSem,0,0);
    }
    ~Student() {
        sem_destroy(&printSem);
    }
};
Student** studentsP;
PrintingStation printingStation[printingStationNum];
void testPrint(int id) {
    if (studentsP[id-1]->state==WAITING_TO_PRINT && printingStation[id%printingStationNum].state==FREE) {
        studentsP[id-1]->state = NOTIFIED;
        printingStation[id%printingStationNum].state = BUSY;
        sem_post(&studentsP[id-1]->printSem);
    }
}
void* normalStudentWork(void* arg)
{
    int* t = (int*) arg;
	int id = *t;
    int startingDelay = distribution(generator);
    sleep(startingDelay);
	pthread_mutex_lock(&printingStation[id%printingStationNum].lock);
	pthread_mutex_lock(&globalPrintlock);
    cout<<"Student "<<id<<" has arrived at print station at time "<<chrono::duration_cast<chrono::seconds> (chrono::system_clock::now()-start).count()<<endl;
    pthread_mutex_unlock(&globalPrintlock);
    studentsP[id-1]->state = WAITING_TO_PRINT;
    testPrint(id);
    pthread_mutex_unlock(&printingStation[id%printingStationNum].lock);
    sem_wait(&studentsP[id-1]->printSem);
    sleep(w);
    pthread_mutex_lock(&printingStation[id%printingStationNum].lock);
    pthread_mutex_lock(&globalPrintlock);
    cout<<"Student "<<id<<" has finished printing at time "<<chrono::duration_cast<chrono::seconds> (chrono::system_clock::now()-start).count()<<endl;
    pthread_mutex_unlock(&globalPrintlock);
    studentsP[id-1]->state = FINISHED;
    printingStation[id%printingStationNum].state = FREE;
    // wake group members
    for (int i=0;i<m;i++) {
        if (((id-1)/m)*m+i+1==id || id%printingStationNum!=(((id-1)/m)*m+i+1)%printingStationNum) continue;
        testPrint(((id-1)/m)*m+i+1);
    }
    // wake others
    for (int i=0;i<n;i++) {
        if (i/m==(id-1)/m || id%printingStationNum!=(i+1)%printingStationNum) continue;
        testPrint(i+1);
    }
    pthread_mutex_unlock(&printingStation[id%printingStationNum].lock);
    return 0;
}
void* leaderStudentWork(void* arg)
{
    int* t = (int*) arg;
	int id = *t;
    normalStudentWork((void*)&id);
    int i = 1;
    while (i<=m-1) {
        pthread_join(studentsP[id-1-i]->thread,NULL);
        i++;
    }
    pthread_mutex_lock(&globalPrintlock);
    cout<<"Group "<<(id-1)/m+1<<" has finished printing at time "<<chrono::duration_cast<chrono::seconds> (chrono::system_clock::now()-start).count()<<endl;
    pthread_mutex_unlock(&globalPrintlock);
    sleep(rand()%4);    // random delay
    //binding
    sem_wait(&bindingStation);
    pthread_mutex_lock(&globalPrintlock);
    cout<<"Group "<<(id-1)/m+1<<" has started binding at time "<<chrono::duration_cast<chrono::seconds> (chrono::system_clock::now()-start).count()<<endl;
    pthread_mutex_unlock(&globalPrintlock);
    sleep(x);
    pthread_mutex_lock(&globalPrintlock);
    cout<<"Group "<<(id-1)/m+1<<" has finished binding at time "<<chrono::duration_cast<chrono::seconds> (chrono::system_clock::now()-start).count()<<endl;
    pthread_mutex_unlock(&globalPrintlock);
    sem_post(&bindingStation);
    //submission
    sleep(rand()%4);    // random delay
    pthread_mutex_lock(&entryBookWriteLock);
    sleep(y);
    submissionCount++;
    pthread_mutex_lock(&globalPrintlock);
    cout<<"Group "<<(id-1)/m+1<<" has submitted the report at time "<<chrono::duration_cast<chrono::seconds> (chrono::system_clock::now()-start).count()<<endl;
    pthread_mutex_unlock(&globalPrintlock);
    pthread_mutex_unlock(&entryBookWriteLock);
    return 0;
}
void* stuffWork(void* arg)
{
    int* t = (int*) arg;
	int id = *t;
	int currentSubmissionCount = 0;
    int startingDelay = rand()%y+2;
	int randomDelay = rand()%4+y+2;
	sleep(startingDelay);
    while (true) {
        pthread_mutex_lock(&entryBookLock);
        readerCount++;
        if (readerCount==1) pthread_mutex_lock(&entryBookWriteLock);
        pthread_mutex_unlock(&entryBookLock);
        pthread_mutex_lock(&globalPrintlock);
        cout<<"Stuff "<<id<<" has started reading the entry book at time "<<chrono::duration_cast<chrono::seconds> (chrono::system_clock::now()-start).count()<<". No. of submission = "<<submissionCount<<endl;
        pthread_mutex_unlock(&globalPrintlock);
        currentSubmissionCount = submissionCount;
        sleep(y);
        pthread_mutex_lock(&entryBookLock);
        readerCount--;
        if (readerCount==0) pthread_mutex_unlock(&entryBookWriteLock);
        pthread_mutex_unlock(&entryBookLock);
        if (currentSubmissionCount>=n/m) break;
        sleep(randomDelay);
    }
    return 0;
}
int main ()
{
    srand(time(0));
    pthread_mutex_init(&globalPrintlock,NULL);
    pthread_mutex_init(&entryBookLock,NULL);
    pthread_mutex_init(&entryBookWriteLock,NULL);
    sem_init(&bindingStation,0,bindingStationNum);
    cin>>n>>m;
    cin>>w>>x>>y;
    studentsP = new Student*[n];
    for (int i=0;i<n;i++) {
        studentsP[i] = new Student(i+1);
    }
    start = chrono::system_clock::now();
    for (int i=0;i<n;i++) {
        int r;
        if (studentsP[i]->id%m==0) {
            r = pthread_create(&studentsP[i]->thread,NULL,leaderStudentWork,(void*)(&studentsP[i]->id));
        }
        else {
            r = pthread_create(&studentsP[i]->thread,NULL,normalStudentWork,(void*)(&studentsP[i]->id));
        }
        if (r) {
            i--;
        }
    }
    pthread_t stuffThread[stuffNum];
    int stuffId[stuffNum];
    for (int i=0;i<stuffNum;i++) {
        stuffId[i] = i+1;
    }
    for (int i=0;i<stuffNum;i++) {
        int r = pthread_create(&stuffThread[i],NULL,stuffWork,(void*)(&stuffId[i]));
        if (r) {
            i--;
        }
    }
    for (int i=0;i<n/m;i++) {
        pthread_join(studentsP[i*m+m-1]->thread,NULL);
    }
    for (int i=0;i<stuffNum;i++) {
        pthread_join(stuffThread[i],NULL);
    }
    for (int i=0;i<n;i++) {
        delete studentsP[i];
    }
    delete [] studentsP;
    pthread_mutex_destroy(&globalPrintlock);
    pthread_mutex_destroy(&entryBookLock);
    pthread_mutex_destroy(&entryBookWriteLock);
    sem_destroy(&bindingStation);
	return 0;
}
