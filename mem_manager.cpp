// OS Project 
// Name: Wajih Hassan Raza
// PSID: 2414696 
// Memory Manager 
// To run: g++ -std=c++17 -O2 mem_manager.cpp -o mem_manager
// To execute: ./mem_manager

#include <bits/stdc++.h>
using namespace std;

// define data structures 

struct PageRef {
    int time;   // global time step
    int pid;    // process id
    int page;   // virtual page num
};

struct Workload {
    string name;
    vector<PageRef> refs;
    int numPages = 0;
    int numProcesses = 0;
};

struct Frame {
    int pid = -1;
    int page = -1;
    int lastUsedTime = -1;  // LRU
    int loadedTime = -1;    // FIFO
    bool valid = false;
};

struct Stats {
    string workloadName;
    string algorithmName;
    int numFrames = 0;
    int windowSize = 0;
    int numRefs = 0;
    int pageFaults = 0;
    double faultRate = 0.0;
    int thrashEvents = 0;
    double avgTotalWS = 0.0;  // average total working-set size across all processes
};

enum class Algorithm { FIFO, LRU, WORKING_SET_LRU };

// Memory Manager section

class MemoryManager {
public:
    MemoryManager(int numFrames, int windowSize, Algorithm alg)
        : numFrames(numFrames), windowSize(windowSize), alg(alg) {
        frames.assign(numFrames, Frame{});
    }

    Stats run(const Workload &w) {
        reset();
        Stats s;
        s.workloadName = w.name;
        s.algorithmName = algorithmName();
        s.numFrames = numFrames;
        s.windowSize = windowSize;
        s.numRefs = (int)w.refs.size();

        for (const auto &r : w.refs) {
            currentTime = r.time;
            bool fault = accessPage(r.pid, r.page);
            if (fault) s.pageFaults++;

            updateWorkingSets(r.pid, r.page);
            bool thrashNow = detectThrashing(fault);
            if (thrashNow) s.thrashEvents++;

            totalWSAccum += currentTotalWS;
        }

        s.faultRate = s.numRefs > 0 ? (double)s.pageFaults / s.numRefs : 0.0;
        s.avgTotalWS = s.numRefs > 0 ? totalWSAccum / s.numRefs : 0.0;
        return s;
    }

private:
    int numFrames;
    int windowSize;            // working-set window
    Algorithm alg;
    vector<Frame> frames;
    int currentTime = 0;

    // for working-set tracking per process
    unordered_map<int, deque<int>> procWindows;              // last W pages for each process id
    unordered_map<int, unordered_map<int,int>> procFreq;     // pid -> (page -> count in window)
    int currentTotalWS = 0;
    double totalWSAccum = 0.0;

    // for page-fault-rate-based thrashing
    deque<bool> faultWindow;   // check last W references: fault or not
    int faultCountInWindow = 0;
    const double faultThreshold = 0.5; // 50%+ faults in last W refs means thrashing

    void reset() {
        for (auto &f : frames) f = Frame{};
        currentTime = 0;
        procWindows.clear();
        procFreq.clear();
        currentTotalWS = 0;
        totalWSAccum = 0.0;
        faultWindow.clear();
        faultCountInWindow = 0;
    }

    string algorithmName() const {
        switch (alg) {
            case Algorithm::FIFO: return "FIFO";
            case Algorithm::LRU: return "LRU";
            case Algorithm::WORKING_SET_LRU: return "WS+LRU";
        }
        return "Unknown";
    }

    // main access function
    bool accessPage(int pid, int page) {
        // check for hit
        for (auto &f : frames) {
            if (f.valid && f.pid == pid && f.page == page) {
                f.lastUsedTime = currentTime;
                return registerFault(false);
            }
        }

        // check for miss: find free frame or choose victim
        int frameIdx = findFreeFrame();
        if (frameIdx == -1) {
            frameIdx = chooseVictim(pid, page);
        }
        loadIntoFrame(frameIdx, pid, page);
        return registerFault(true);
    }

    // tracking faults in sliding window approach
    bool registerFault(bool isFault) {
        faultWindow.push_back(isFault);
        if (isFault) faultCountInWindow++;

        if ((int)faultWindow.size() > windowSize) {
            if (faultWindow.front()) faultCountInWindow--;
            faultWindow.pop_front();
        }
        return isFault;
    }

    int findFreeFrame() {
        for (int i = 0; i < numFrames; ++i) {
            if (!frames[i].valid) return i;
        }
        return -1;
    }

    // choosing victim based on algorithm
    int chooseVictim(int pid, int page) {
        // FIFO: smallest loadtime frame selected
        if (alg == Algorithm::FIFO) {
            int idx = 0;
            int bestTime = frames[0].loadedTime;
            for (int i = 1; i < numFrames; ++i) {
                if (frames[i].loadedTime < bestTime) {
                    bestTime = frames[i].loadedTime;
                    idx = i;
                }
            }
            return idx;
        }

        // LRU victim
        auto lruVictim = [&]() {
            int idx = 0;
            int bestTime = frames[0].lastUsedTime;
            for (int i = 1; i < numFrames; ++i) {
                if (frames[i].lastUsedTime < bestTime) {
                    bestTime = frames[i].lastUsedTime;
                    idx = i;
                }
            }
            return idx;
        };

        if (alg == Algorithm::LRU) {
            return lruVictim();
        }

        // WORKING_SET_LRU:
        // Try to evict a page that is not in its owner's current working set
        int candidate = -1;
        for (int i = 0; i < numFrames; ++i) {
            if (!frames[i].valid) continue;
            int owner = frames[i].pid;
            int ownerPage = frames[i].page;

            auto itProc = procFreq.find(owner);
            bool inWS = false;
            if (itProc != procFreq.end()) {
                auto itPage = itProc->second.find(ownerPage);
                inWS = (itPage != itProc->second.end() && itPage->second > 0);
            }
            if (!inWS) {
                candidate = i;
                break;
            }
        }
        if (candidate != -1) return candidate;

        // if all frames are holding pages in their working sets: fall back to LRU
        return lruVictim();
    }

    void loadIntoFrame(int idx, int pid, int page) {
        frames[idx].pid = pid;
        frames[idx].page = page;
        frames[idx].loadedTime = currentTime;
        frames[idx].lastUsedTime = currentTime;
        frames[idx].valid = true;
    }

    // maintain working set windows per process
    void updateWorkingSets(int pid, int page) {
        auto &win = procWindows[pid];   // deque of last W pages
        auto &freq = procFreq[pid];     // page -> count over that window

        win.push_back(page);
        freq[page]++;

        if ((int)win.size() > windowSize) {
            int old = win.front();
            win.pop_front();
            auto it = freq.find(old);
            if (it != freq.end()) {
                it->second--;
                if (it->second == 0) freq.erase(it);
            }
        }

        // recompute total working-set size across all processes
        currentTotalWS = 0;
        for (const auto &kv : procFreq) {
            currentTotalWS += (int)kv.second.size();
        }
    }

    // detect thrashing
    bool detectThrashing(bool /*lastWasFault*/) {
        bool wsTooBig = currentTotalWS > numFrames;
        bool faultRateHigh = false;

        if (!faultWindow.empty()) {
            double rate = (double)faultCountInWindow / faultWindow.size();
            faultRateHigh = rate > faultThreshold;
        }
        return wsTooBig || faultRateHigh;
    }
};

// Testing
// Sequential scan: process 0 scans pages [0..numPages-1] repeatedly
Workload make_sequential(int pid, int numPages, int repeats) {
    Workload w;
    w.name = "Sequential_scan";
    int t = 0;
    for (int r = 0; r < repeats; ++r) {
        for (int p = 0; p < numPages; ++p) {
            w.refs.push_back({t++, pid, p});
        }
    }
    w.numPages = numPages;
    w.numProcesses = 1;
    return w;
}

// Locality loop: process 0 walks through small chunks inside a larger address space
Workload make_locality_loop(int pid, int numPages, int localitySize, int length) {
    Workload w;
    w.name = "Locality_loop";
    int t = 0;
    int start = 0;
    for (int i = 0; i < length; ++i) {
        int offset = i % localitySize;
        int page = (start + offset) % numPages;

        // Occasionally shift locality window
        if (i % (2 * localitySize) == 0) {
            start = (start + localitySize) % max(numPages, 1);
        }
        w.refs.push_back({t++, pid, page});
    }
    w.numPages = numPages;
    w.numProcesses = 1;
    return w;
}

// Random references: bad locality, stress test for algorithms
Workload make_random(int pid, int numPages, int length, unsigned seed) {
    Workload w;
    w.name = "Random";
    mt19937 gen(seed);
    uniform_int_distribution<int> dist(0, numPages - 1);
    int t = 0;
    for (int i = 0; i < length; ++i) {
        int page = dist(gen);
        w.refs.push_back({t++, pid, page});
    }
    w.numPages = numPages;
    w.numProcesses = 1;
    return w;
}

// Mixed multiprogramming: two processes, one with good locality and one random
Workload make_multiprogrammed_mixture() {
    Workload w;
    w.name = "Mixed_multiprogrammed";

    int t = 0;
    int length = 400;
    int numPagesP0 = 32;
    int numPagesP1 = 32;
    mt19937 gen(42);
    uniform_int_distribution<int> dist(0, numPagesP1 - 1);

    for (int i = 0; i < length; ++i) {
        // process 0: strong locality in first 8 pages
        int page0 = (i % 8);
        w.refs.push_back({t++, 0, page0});

        // process 1: more random behavior
        int page1 = dist(gen);
        w.refs.push_back({t++, 1, page1});
    }

    w.numPages = max(numPagesP0, numPagesP1);
    w.numProcesses = 2;
    return w;
}

// printing all utility vals

void printStatsHeader() {
    cout << left << setw(22) << "Workload"
         << setw(10) << "Alg"
         << setw(8)  << "Frames"
         << setw(8)  << "Win"
         << setw(10) << "Refs"
         << setw(12) << "Faults"
         << setw(12) << "FaultRate"
         << setw(14) << "ThrashEvt"
         << setw(12) << "AvgTotWS" << "\n";
    cout << string(100, '-') << "\n";
}

void printStats(const Stats &s) {
    cout << left << setw(22) << s.workloadName
         << setw(10) << s.algorithmName
         << setw(8)  << s.numFrames
         << setw(8)  << s.windowSize
         << setw(10) << s.numRefs
         << setw(12) << s.pageFaults
         << setw(12) << fixed << setprecision(3) << s.faultRate
         << setw(14) << s.thrashEvents
         << setw(12) << fixed << setprecision(2) << s.avgTotalWS
         << "\n";
}

// CSV helpers

void writeCsvHeader(ofstream &csv) {
    csv << "Experiment"
        << ",Workload"
        << ",Alg"
        << ",Frames"
        << ",Win"
        << ",Refs"
        << ",Faults"
        << ",FaultRate"
        << ",ThrashEvt"
        << ",AvgTotWS"
        << "\n";
}

void writeCsvRow(ofstream &csv, const string &experimentName, const Stats &s) {
    csv << experimentName << ","
        << s.workloadName << ","
        << s.algorithmName << ","
        << s.numFrames << ","
        << s.windowSize << ","
        << s.numRefs << ","
        << s.pageFaults << ","
        << fixed << setprecision(6) << s.faultRate << ","
        << s.thrashEvents << ","
        << fixed << setprecision(6) << s.avgTotalWS
        << "\n";
}

// Running benchmarks (main)

int main() {
    // Global simulation parameters for baseline experiment
    int baseNumFrames  = 16;  // total physical frames
    int baseWindowSize = 10;  // working-set window size

    // build workloads once
    vector<Workload> workloads;
    workloads.push_back(make_sequential(0, 32, 2));          
    workloads.push_back(make_locality_loop(0, 64, 10, 400)); 
    workloads.push_back(make_random(0, 64, 400, 123));       
    workloads.push_back(make_multiprogrammed_mixture());  

    vector<Algorithm> algs = {
        Algorithm::FIFO,
        Algorithm::LRU,
        Algorithm::WORKING_SET_LRU
    };

    // open csv file
    ofstream csv("results.csv");
    if (!csv.is_open()) {
        cerr << "Error: could not open results.csv for writing.\n";
        return 1;
    }
    writeCsvHeader(csv);

    // ============================
    // Experiment 1: Baseline setup
    // ============================
    cout << "===== Experiment 1: Baseline (Frames = " << baseNumFrames
         << ", Window = " << baseWindowSize << ") =====\n";
    printStatsHeader();
    for (const auto &w : workloads) {
        for (auto alg : algs) {
            MemoryManager mm(baseNumFrames, baseWindowSize, alg);
            Stats s = mm.run(w);
            printStats(s);
            writeCsvRow(csv, "Baseline", s);
        }
    }

    // =======================================
    // Experiment 2: Frames sweep (for plots)
    // =======================================
    cout << "\n\n===== Experiment 2: Frames Sweep (Window = "
         << baseWindowSize << ") =====\n";
    vector<int> frameOptions = {8, 12, 16, 24, 32};
    printStatsHeader();
    for (int nf : frameOptions) {
        for (const auto &w : workloads) {
            for (auto alg : algs) {
                MemoryManager mm(nf, baseWindowSize, alg);
                Stats s = mm.run(w);
                printStats(s);
                writeCsvRow(csv, "FramesSweep", s);
            }
        }
    }

    // ===================================================
    // Experiment 3: Working-set window sweep (WS+LRU only)
    // ===================================================
    cout << "\n\n===== Experiment 3: Working-Set Window Sweep "
         << "(Alg = WS+LRU, Workload = Locality_loop, Frames = "
         << baseNumFrames << ") =====\n";

    // find the Locality_loop workload
    Workload localityWorkload;
    bool foundLocality = false;
    for (const auto &w : workloads) {
        if (w.name == "Locality_loop") {
            localityWorkload = w;
            foundLocality = true;
            break;
        }
    }

    if (foundLocality) {
        vector<int> windowOptions = {5, 10, 20, 40};
        printStatsHeader();
        for (int win : windowOptions) {
            MemoryManager mm(baseNumFrames, win, Algorithm::WORKING_SET_LRU);
            Stats s = mm.run(localityWorkload);
            printStats(s);
            writeCsvRow(csv, "WindowSweep", s);
        }
    } else {
        cout << "Locality_loop workload not found, skipping Experiment 3.\n";
    }

    csv.close();
    cout << "\nResults written to results.csv\n";
    return 0;
}
