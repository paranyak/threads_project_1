#include <iostream>
#include <map>
#include <vector>
#include <math.h>
#include <assert.h>
#include <thread>
#include <mutex>
#include <deque>
#include <condition_variable>


using namespace std;
mutex myMutex;
mutex mx;
condition_variable cv;


int num = 0;
int num_of_threads = 0;
//

void printMap(const map<string, int> &m) {
    for (auto &elem : m) {
        cout << elem.first << " " << elem.second << "\n";
    }
}

//

map<string, int> reduce_f( map<string, int> m, const map<string, int>& localm ) {

    for (auto it = localm.begin(); it != localm.end(); ++it)
        m[it->first] += it->second;
    return m;
}
template <class Dd, class RF>
void reducer(deque <Dd> &dm, RF (*fn2)(Dd, const Dd &)){
    unique_lock<mutex> uniqueLock(myMutex);
    while (dm.size() > 1) {
        Dd map1 = dm.front();
        dm.pop_front();
        Dd map2 = dm.front();
        dm.pop_front();
        uniqueLock.unlock();
        Dd map3 = (*fn2)(map1, map2);

        uniqueLock.lock();
        dm.push_back(map3);
//        cout << "1" << endl;
    }if (dm.size() == 1 && num == num_of_threads) {
        printMap(dm.front());
//        cout << "2" << endl;
    } else {
//        cout << "3" << endl;
        cv.wait(uniqueLock);
    }

}
map<string, int> counting_words_worker(const vector<string>::const_iterator &beg, const vector<string>::const_iterator &fin , deque <map<string, int>> & dm, map<string, int> (*reduce_f)( map<string, int> , const map<string, int> &)){
    map<string, int> localm;
    for (auto i = beg; i != fin; ++i) {
        ++localm[*i];
    }
    {
        lock_guard<mutex> lg(myMutex);
        dm.push_back(localm);
    }
    cv.notify_one();
    cv.notify_all();
    num += 1;
    reducer(dm, reduce_f);
    return localm;
}



template <class MF, class RF, class I, class N, class D>
auto func_tmpl(I beg, I fin, MF fn1, D d,  RF fn2,  N num_of_threads) -> decltype( fn1(beg, fin, d, fn2))  {
    decltype(func_tmpl(beg, fin, fn1, d,  fn2, num_of_threads)) res;        // big map<string, int>

    size_t delta = (fin - beg) / num_of_threads;
    vector<I> segments;
    for (auto i = beg; i < fin - delta; i += delta) {
        segments.push_back(i);
    }
    segments.push_back(fin);

    /////////////////////


    thread myThreads [num_of_threads];
    for (auto i = segments.begin(); i < segments.end()-1; ++i) {
        myThreads[i - segments.begin()] = thread(fn1,*i, *(i+1), ref(d), fn2);

    }

    for (auto& th : myThreads) th.join();
    return res;
}






int main()
{
    vector<string> v = {"aaaa", "hhhhh", "bbbbb", "ccccc", "ddddd", "aaaa", "eeeee", "wwwwwwwww", "ccccc", "ccccc", "ccccc"};
    cout<<"Words counter" << endl;
    num_of_threads = 3;
    deque <map<string, int>> dm;
    func_tmpl(v.begin(), v.end(), counting_words_worker, dm, reduce_f, num_of_threads);

}


