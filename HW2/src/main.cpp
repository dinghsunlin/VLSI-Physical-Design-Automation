#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>

#define __IN_A__    0
#define __IN_B__    1

using namespace std;

int cut_size = 0;
unsigned area_A = 0, area_B = 0;

map<int, unordered_set<string>, greater<int>> gain_A, gain_B;
unordered_map<string, array<int, 2>> cell_area, front_to;
unordered_map<string, int> group_A, group_B;
unordered_map<string, unordered_set<string>> cell_nets, net_cells;

bool check_area_constraint() {
    unsigned diff = (area_A >= area_B) ? (area_A - area_B) : (area_B - area_A);
    unsigned sum = area_A + area_B;
    return (diff < sum / 10);
}

bool check_area_constraint(array<int, 2> & area) {
    unsigned diff = (area_A >= area_B) ? (area_A - area_B) : (area_B - area_A);
    unsigned sum = area_A + area_B;
    if(diff < sum / 10)
        return true;
    else {
        area_A += area[__IN_A__];
        area_B -= area[__IN_B__];
        return false;
    }
}

bool check_area_constraint(string cell) {
    array<int, 2> area = cell_area[cell];
    if(group_A.find(cell) != group_A.end()) {
        area_A -= area[__IN_A__];
        area_B += area[__IN_B__];
        unsigned diff = (area_A >= area_B) ? (area_A - area_B) : (area_B - area_A);
        unsigned sum = area_A + area_B;
        if(diff < sum / 10) {
            group_A.erase(cell);
            group_B.emplace(cell, NULL);
            return true;
        }
        else {
            area_A += area[__IN_A__];
            area_B -= area[__IN_B__];
            return false;
        }
    }
    else {
        area_A += area[__IN_A__];
        area_B -= area[__IN_B__];
        unsigned diff = (area_A >= area_B) ? (area_A - area_B) : (area_B - area_A);
        unsigned sum = area_A + area_B;
        if(diff < sum / 10) {
            group_A.emplace(cell, NULL);
            group_B.erase(cell);
            return true;
        }
        else {
            area_A -= area[__IN_A__];
            area_B += area[__IN_B__];
            return false;
        }
    }
}

void initial_partition() {
    multimap<int, string, greater<int>> tmp;
    for(const auto & i : cell_area) {
        string cell = i.first;
        array<int, 2> area = i.second;
        area_A += area[__IN_A__];

        tmp.emplace(area[__IN_A__] + area[__IN_B__], cell);
    }

    auto now = tmp.begin();
    do {
        string cell = now->second;
        array<int, 2> area = cell_area[cell];
        area_A -= area[__IN_A__];
        area_B += area[__IN_B__];
        group_B.emplace(cell, 0);
        now = tmp.erase(now);

        for(const string & net : cell_nets[cell]) {
            auto search = front_to.find(net);
            array<int, 2> nums = {0, 1};
            if(search != front_to.end()) {
                nums = search->second;
                nums[__IN_B__]++;
            }
            front_to[net] = nums;
        }
    }
    while(!check_area_constraint());

    while(true) {
        string cell = now->second;
        array<int, 2> area = cell_area[cell];
        area_A -= area[__IN_A__];
        area_B += area[__IN_B__];
        // if(area_A <= area_B) {
        //     area_A += area[__IN_A__];
        //     area_B -= area[__IN_B__];
        //     break;
        // }
        if(!check_area_constraint(area))
            break;
        group_B.emplace(cell, 0);
        now = tmp.erase(now);
    
        for(const string & net : cell_nets[cell]) {
            auto search = front_to.find(net);
            array<int, 2> nums = {0, 1};
            if(search != front_to.end()) {
                nums = search->second;
                nums[__IN_B__]++;
            }
            front_to[net] = nums;
        }
    }

    do {
        string cell = now->second;
        group_A.emplace(cell, 0);
        now = tmp.erase(now);

        for(const string & net : cell_nets[cell]) {
            auto search = front_to.find(net);
            array<int, 2> nums = {1, 0};
            if(search != front_to.end()) {
                nums = search->second;
                if((nums[__IN_A__]++ == 0) && (nums[__IN_B__] > 0))
                    cut_size++;
            }
            front_to[net] = nums;
        }
    }
    while(now != tmp.end());
}

void initial_gain() {
    if(!gain_A.empty()) {
        for(auto & i : gain_A)
            i.second.clear();
        gain_A.clear();
    }
    if(!gain_B.empty()) {
        for(auto & i : gain_B)
            i.second.clear();
        gain_B.clear();
    }

    for(auto & i : group_A) {
        const string & cell = i.first;
        int & value = i.second = 0;
        for(const string & net : cell_nets[cell]) {
            if(front_to[net][__IN_A__] == 1)
                value++;
            if(front_to[net][__IN_B__] == 0)
                value--;
        }
        if(value >= 0)
            gain_A[value].emplace(cell);
        else if(value == -1 && gain_A[value].size() < 4)
            gain_A[value].emplace(cell);
    }

    for(auto & i : group_B) {
        const string & cell = i.first;
        int & value = i.second = 0;
        for(const string & net : cell_nets[cell]) {
            if(front_to[net][__IN_B__] == 1)
                value++;
            if(front_to[net][__IN_A__] == 0)
                value--;
        }

        if(value >= 0)
            gain_B[value].emplace(cell);
        else if(value == -1 && gain_B[value].size() < 4)
            gain_B[value].emplace(cell);
    }
}

void update_gain(string cell) {
    short group = (group_A.find(cell) == group_A.end()) ? __IN_A__ : __IN_B__;
    map<int, unordered_set<string>, greater<int>> & front_gain = (group == __IN_A__) ? gain_A : gain_B;
    map<int, unordered_set<string>, greater<int>> & to_gain = (group == __IN_A__) ? gain_B : gain_A;
    unordered_map<string, int> & front_group = (group == __IN_A__) ? group_A : group_B;
    unordered_map<string, int> & to_group = (group == __IN_A__) ? group_B : group_A;
    for(const string & net : cell_nets[(cell)]) {
        net_cells[net].erase(cell);
        if(front_to[net][!group] == 0) {
            cut_size++;
            for(const string & name : net_cells[net]) {
                int & update = front_group[name];
                if(update >= 0) {
                    front_gain[update].erase(name);
                    if(front_gain[update].empty())
                        front_gain.erase(update);
                }
                update++;
                if(update >= 0)
                    front_gain[update].emplace(name);
                else if(update == -1 && front_gain[update].size() < 4)
                    front_gain[update].emplace(name);
            }
        }
        else if(front_to[net][!group] == 1) {
            for(const string & name : net_cells[net]) {
                auto search = to_group.find(name);
                if(search != to_group.end()) {
                    int & update = search->second;
                    if(update >= 0) {
                        to_gain[update].erase(name);
                        if(to_gain[update].empty())
                            to_gain.erase(update);
                    }
                    update--;
                    if(update >= 0)
                        to_gain[update].emplace(name);
                    else if(update == -1 && to_gain[update].size() < 4)
                        to_gain[update].emplace(name);
                    break;
                }
            }
        }
        front_to[net][group]--;
        front_to[net][!group]++;
        if(front_to[net][group] == 0) {
            cut_size--;
            for(const string & name : net_cells[net]) {
                int & update = to_group[name];
                if(update >= 0) {
                    to_gain[update].erase(name);
                    if(to_gain[update].empty())
                        to_gain.erase(update);
                }
                update--;
                if(update >= 0)
                    to_gain[update].emplace(name);
                else if(update == -1 && to_gain[update].size() < 4)
                    to_gain[update].emplace(name);
            }
        }
        else if(front_to[net][group] == 1) {
            for(const string & name : net_cells[net]) {
                auto search = front_group.find(name);
                if(search != front_group.end()) {
                    int & update = search->second;
                    if(update >= 0) {
                        front_gain[update].erase(name);
                        if(front_gain[update].empty())
                            front_gain.erase(update);
                    }
                    update++;
                    if(update >= 0)
                        front_gain[update].emplace(name);
                    else if(update == -1 && front_gain[update].size() < 4)
                        front_gain[update].emplace(name);
                    break;
                }
            }
        }
    }
}

int main(int argc, char * argv[]) {
    auto input_start = chrono::steady_clock::now();

    cell_area.reserve(1000000);
    cell_nets.reserve(1000000);
    net_cells.reserve(1000000);

    fstream cells;
    cells.open(argv[1], ios::in);
    while(!cells.eof()) {
        string cell;
        int area_in_A, area_in_B;
        cells >> cell >> area_in_A >> area_in_B;
        
        if(cell.empty())
            break;

        cell_area.emplace(cell, array<int, 2>{area_in_A, area_in_B});
    }
    cells.close();

    fstream nets;
    nets.open(argv[2], ios::in);
    while(!nets.eof()) {
        string net, cell;
        nets >> net >> net >> cell >> cell;

        if(net.empty() && cell.empty())
            break;

        unordered_set<string> in_net;
        while(cell != "}") {
            in_net.emplace(cell);
            cell_nets[cell].emplace(net);

            nets >> cell;
        }
        net_cells[net] = in_net;
    }
    nets.close();

    auto input_end = chrono::steady_clock::now();
    auto compute_start = chrono::steady_clock::now();
    
    group_A.reserve(1000000);
    group_B.reserve(1000000);
    front_to.reserve(1000000);

    initial_partition();
    initial_gain();

    int last_cut_size = cut_size;
    unordered_map<string, unordered_set<string>> tmp_net_cells = net_cells;
    unordered_map<string, int> last_group_A = group_A, last_group_B = group_B;

    int iteration = 0, repeat = 0;
    while(true) {
        cout << " - Iteration " << ++iteration;

        map<int, unordered_set<string>, greater<int>> & first = (area_A >= area_B) ? gain_B : gain_A;
        map<int, unordered_set<string>, greater<int>> & second = (area_A >= area_B) ? gain_A : gain_B;
        bool cont = true;
        while(cont) {
            cont = false;
            for(auto & i : first) {
                for(auto & j : i.second) {
                    if(check_area_constraint(j)) {
                        string cell = j;
                        cont = true;
                        i.second.erase(cell);
                        if(i.second.empty())
                            first.erase(i.first);
                        update_gain(cell);
                        break;
                    }
                }
                if(cont)
                    break;
            }
        }
        cont = true;
        while(cont) {
            cont = false;
            for(auto & i : second) {
                for(auto & j : i.second) {
                    if(check_area_constraint(j)) {
                        string cell = j;
                        cont = true;
                        i.second.erase(cell);
                        if(i.second.empty())
                            second.erase(i.first);
                        update_gain(cell);
                        break;
                    }
                }
                if(cont)
                    break;
            }
        }

        if(cut_size == last_cut_size)
            repeat++;
        else
            repeat = 0;

        if(cut_size <= last_cut_size && repeat < 5) {
            cout << "   |   cut size: " << cut_size << endl;
            last_cut_size = cut_size;
            last_group_A = group_A;
            last_group_B = group_B;
            net_cells = tmp_net_cells;
            initial_gain();
        }
        else {
            cout << "   |   cut size: " << cut_size << endl << endl;
            last_cut_size = cut_size;
            net_cells = tmp_net_cells;
            initial_gain();
            break;
        }
    }
    while(true) {
        auto iter_start = chrono::steady_clock::now();
        cout << " - Iteration " << ++iteration;

        bool cont = true;
        while(cont) {
            cont = false;
            map<int, unordered_set<string>, greater<int>> & first = (area_A > area_B) ? gain_A : gain_B;
            map<int, unordered_set<string>, greater<int>> & second = (area_A > area_B) ? gain_B : gain_A;
            int now_gain = (first.begin()->first >= second.begin()->first) ? first.begin()->first : second.begin()->first;
            while(now_gain >= -1) {
                if(first.find(now_gain) != first.end()) {
                    for(auto & j : first[now_gain]) {
                        if(check_area_constraint(j)) {
                            string cell = j;
                            cont = true;
                            first[now_gain].erase(cell);
                            if(first[now_gain].empty())
                                first.erase(now_gain);
                            update_gain(cell);
                            now_gain = (first.begin()->first >= second.begin()->first) ? first.begin()->first : second.begin()->first;
                            break;
                        }
                    }
                    if(cont)
                        break;
                }
                if(second.find(now_gain) != second.end()) {
                    for(auto & j : second[now_gain]) {
                        if(check_area_constraint(j)) {
                            string cell = j;
                            cont = true;
                            second[now_gain].erase(cell);
                            if(second[now_gain].empty())
                                second.erase(now_gain);
                            update_gain(cell);
                            now_gain = (first.begin()->first >= second.begin()->first) ? first.begin()->first : second.begin()->first;
                            break;
                        }
                    }
                    if(cont)
                        break;
                }
                now_gain--;
            }
        }

        if(cut_size == last_cut_size)
            repeat++;
        else
            repeat = 0;

        if(cut_size <= last_cut_size && repeat < 5) {
            cout << "   |   cut size: " << cut_size << endl;
            last_cut_size = cut_size;
            last_group_A = group_A;
            last_group_B = group_B;
            net_cells = tmp_net_cells;
            initial_gain();
        }
        else {
            cout << "   |   cut size: " << cut_size << endl;
            cut_size = last_cut_size;
            group_A = last_group_A;
            group_B = last_group_B;
            break;
        }
        
        auto iter_end = chrono::steady_clock::now();
        if((295 - chrono::duration<float>(iter_end - input_start).count()) < 3 * chrono::duration<float>(iter_end - iter_start).count())
            break;
    }

    auto compute_end = chrono::steady_clock::now();
    auto output_start = chrono::steady_clock::now();

    fstream out;
    out.open(argv[3], ios::out);
    out << "cut_size " << cut_size << endl;
    out << "A " << group_A.size() << endl;
    for(const auto & i : group_A)
        out << i.first << endl;
    out << "B " << group_B.size() << endl;
    for(const auto & i : group_B)
        out << i.first << endl;
    out.close();

    auto output_end = chrono::steady_clock::now();
    // cout << area_A << ' ' << area_B << ' ' << cell_area.size() << endl;
    cout << "----- Timing Result -----\n";
    cout << "  I/O Time:\t\t" << chrono::duration<float>(input_end - input_start + output_end - output_start).count() << "\tsec." << endl;
    cout << "+ Computation Time:\t" << chrono::duration<float>(compute_end - compute_start).count() << "\tsec." << endl;
    cout << "= Total Runtime:\t" << chrono::duration<float>(output_end - input_start).count() << "\tsec." << endl;

    return 0;
}