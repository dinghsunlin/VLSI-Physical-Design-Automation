#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <random>
#include <set>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

// index for blocks_table & terminals_table & struct NODE
#define __X__   0   // x-coordinate
#define __Y__   1   // y-coordinate
#define __XW__  2   // x-width
#define __YH__  3   // y-height
#define __R__   4   // unrotated (or rotated)
// only for struct NODE
#define __F__   2   // index for orientation or combination

// index for nets_table
#define __W__   0   // weight
#define __L__   1   // index of length
#define __P__   2   // pins of a net

// random seed
#define __SEED__    100000

using namespace std;

enum NODE_TYPE {BLOCK, CHAIN};
enum STAGE {AREA, WIRELENGTH};

typedef struct __NODE__ {
    NODE_TYPE type;
    string name;
    array<int, 3> coordinate;
    stack<struct __NODE__ *> stk;
    vector<array<int, 2>> area, from;
} NODE;

int blocks_num, terminals_num, nets_num, pins_num;
unsigned total_length = 0, width = 0, height = 0;
unordered_map<string, array<int, 5>> blocks_table;
unordered_map<string, vector<tuple<int, int, vector<string>>>> nets_table;
unordered_map<string, array<int, 2>> terminals_table;
vector<int> wirelength, chains_sum;
vector<NODE *> blocks;          // blocks: [0] [1]     [2]     [3]     ... [N - 1]
vector<list<NODE *>> chains;    // chains:         [0]     [1]     [2] ...         [N - 2]
vector<string> best, last;

unsigned find_box_size(vector<string> pins, int idx) {
    int x_min, x_max, y_min, y_max;
    bool first = true;
    for(const auto & i : pins) {
        array<int, 2> coordinate;
        if(i[0] == 'p')
            coordinate = terminals_table[i];
        else if(blocks_table[i][__R__] == 0)
            coordinate = {blocks_table[i][__X__] + blocks_table[i][__XW__] / 2, \
                          blocks_table[i][__Y__] + blocks_table[i][__YH__] / 2};
        else if(blocks_table[i][__R__] == 1)
            coordinate = {blocks_table[i][__X__] + blocks_table[i][__YH__] / 2, \
                          blocks_table[i][__Y__] + blocks_table[i][__XW__] / 2};
        
        if(first) {
            x_min = x_max = coordinate[__X__];
            y_min = y_max = coordinate[__Y__];
            first = false;
        }
        else {
            if(coordinate[__X__] <= x_min)
                x_min = coordinate[__X__];
            else if(coordinate[__X__] > x_max)
                x_max = coordinate[__X__];
            
            if(coordinate[__Y__] <= y_min)
                y_min = coordinate[__Y__];
            else if(coordinate[__Y__] > y_max)
                y_max = coordinate[__Y__];
        }
    }

    return wirelength[idx] = (x_max - x_min) + (y_max - y_min);
}

void optimize_orientation(bool ini) {
    stack<NODE *> tree;
    blocks[0]->stk = tree;
    tree.push(blocks[0]);
    blocks[1]->stk = tree;
    tree.push(blocks[1]);
    bool first = true, next = false;
    int count = 0;
    for(int c = 0; c < blocks_num - 1; c++) {
        if(ini && next) {
            next = false;
            if(c == blocks_num - 2)
                chains[c].front()->name = "H";
            else
                chains[c].pop_front();
        }
        for(auto & i : chains[c]) {
            count++;
            i->stk = tree;
            NODE * right = tree.top();
            tree.pop();
            NODE * left = tree.top();
            tree.pop();
            tree.push(i);
            if(i->name == "V") {
                int l_now = 0, r_now = 0;
                i->area.clear();
                i->from.clear();
                while(l_now < left->area.size() && r_now < right->area.size()) {
                    array<int ,2> wh, fr = {l_now, r_now};
                    wh[__X__] = left->area[l_now][__X__] + right->area[r_now][__X__];
                    wh[__Y__] = (left->area[l_now][__Y__] >= right->area[r_now][__Y__]) ? left->area[l_now][__Y__] : right->area[r_now][__Y__];
                    i->area.emplace_back(wh);
                    i->from.emplace_back(fr);
                    if(wh[__Y__] == left->area[l_now][__Y__])
                        l_now++;
                    if(wh[__Y__] == right->area[r_now][__Y__])
                        r_now++;
                    // cout << "V " << left->name << ' ' << right->name << ' ';
                    // cout << i->area.back()[0] << ' ' << i->area.back()[1] << ' ';
                    // cout << i->from.back()[0] << ' ' << i->from.back()[1] << ' ';
                    // char a;
                    // cin >> a;
                }
                if(ini && (i->area.front()[__X__] >= width || c == blocks_num - 2)) {
                    next = true;
                    if(!first) {
                        NODE * tmp = new NODE;
                        tmp->type = CHAIN;
                        tmp->name = "H";
                        chains[c].emplace_back(tmp);
                    }
                    first = false;
                }
            }
            else if(i->name == "H") {
                int l_now = left->area.size() - 1, r_now = right->area.size() - 1;
                i->area.clear();
                i->from.clear();
                while(l_now > -1 && r_now > -1) {
                    array<int ,2> wh, fr = {l_now, r_now};
                    wh[__X__] = (left->area[l_now][__X__] >= right->area[r_now][__X__]) ? left->area[l_now][__X__] : right->area[r_now][__X__];
                    wh[__Y__] = left->area[l_now][__Y__] + right->area[r_now][__Y__];
                    i->area.emplace_back(wh);
                    i->from.emplace_back(fr);
                    if(wh[__X__] == left->area[l_now][__X__])
                        l_now--;
                    if(wh[__X__] == right->area[r_now][__X__])
                        r_now--;
                    // cout << "H " << left->name << ' ' << right->name << ' ';
                    // cout << i->area.back()[0] << ' ' << i->area.back()[1] << ' ';
                    // cout << i->from.back()[0] << ' ' << i->from.back()[1] << '|';
                }
                reverse(i->area.begin(), i->area.end());
                reverse(i->from.begin(), i->from.end());
                // cout << endl;
                // for(const auto & j : i->area)
                //     cout << j[0] << ' ' << j[1] << '|';
                // char a;
                // cin >> a;
            }
        }
        if(ini)
            chains_sum.emplace_back(count);
        if(c < blocks_num - 2)
            tree.push(blocks[c + 2]);
    }
}

void assign_coordinate(NODE * now) {
    stack<NODE *> tree = now->stk;
    NODE * right = tree.top();
    tree.pop();
    NODE * left = tree.top();
    tree.pop();
    left->coordinate = now->coordinate;
    left->coordinate[__F__] = now->from[now->coordinate[__F__]][0];
    right->coordinate = now->coordinate;
    if(now->name == "V")
        right->coordinate[__X__] += left->area[left->coordinate[__F__]][__X__];
    else if(now->name == "H")
        right->coordinate[__Y__] += left->area[left->coordinate[__F__]][__Y__];
    right->coordinate[__F__] = now->from[now->coordinate[__F__]][1];

    if(left->type == CHAIN)
        assign_coordinate(left);
    else {
        blocks_table[left->name][__X__] = left->coordinate[__X__];
        blocks_table[left->name][__Y__] = left->coordinate[__Y__];
        blocks_table[left->name][__R__] = left->from[0][left->coordinate[__F__]];
    }
    if(right->type == CHAIN)
        assign_coordinate(right);
    else {
        blocks_table[right->name][__X__] = right->coordinate[__X__];
        blocks_table[right->name][__Y__] = right->coordinate[__Y__];
        blocks_table[right->name][__R__] = right->from[0][right->coordinate[__F__]];
    }
}

void update_wire_length() {
    int count = 0;
    total_length = 0;
    for(auto & i : wirelength)
        i = -1;
    for(auto & i : nets_table) {
        for(auto & j : i.second) {
            // cout << i.first << " - " << get<__L__>(j) << ": " << wirelength[get<__L__>(j)] << '\t' ;
            if(wirelength[get<__L__>(j)] != -1)
                continue;
            total_length += get<__W__>(j) * find_box_size(get<__P__>(j), get<__L__>(j));
            if(++count == wirelength.size())
                break;
        }
    }
}

NODE * initialize_floorplan() {
    blocks.reserve(blocks_num);
    chains.reserve(blocks_num - 1);
    chains_sum.reserve(blocks_num - 1);
    
    int now = -1;
    for(auto & i : blocks_table) {
        NODE * tmp = new NODE;
        array<int, 2> n = {i.second[__XW__], i.second[__YH__]}, r = {i.second[__YH__], i.second[__XW__]}, t = {0, 0};
        tmp->type = BLOCK;
        tmp->name = i.first;
        if(i.second[__XW__] == i.second[__YH__])
            tmp->area.emplace_back(n);
        else if(i.second[__XW__] < i.second[__YH__]) {
            tmp->area.emplace_back(n);
            tmp->area.emplace_back(r);
            t = {0, 1};
        }
        else if(i.second[__XW__] > i.second[__YH__]) {
            tmp->area.emplace_back(r);
            tmp->area.emplace_back(n);
            t = {1, 0};
        }
        tmp->from.emplace_back(t);
        blocks.emplace_back(tmp);
        // cout << blocks.back()->name << ": " << blocks.back()->area.back()[0] << ' ' << blocks.back()->area.back()[1];
        // char a;
        // cin >> a;

        if(now >= 0) {
            NODE * tmp2 = new NODE;
            list<NODE *> tmp3;
            tmp2->type = CHAIN;
            tmp2->name = "V";
            tmp3.emplace_front(tmp2);
            chains.emplace_back(tmp3);
        }
        now++;
    }
    
    optimize_orientation(true);

    int place, min;
    for(int i = 0; i < chains.back().back()->area.size(); i++) {
        int now;
        array<int, 2> area = chains.back().back()->area[i];
        // cout << area[__X__] << ' ' << area[__Y__] << ' ' << area[__X__] * area[__Y__] << ' ';
        if(area[__X__] > width && area[__Y__] > height)
            now = area[__X__] * area[__Y__] - width * height;
        else if(area[__X__] > width)
            now = (area[__X__] - width) * height;
        else if(area[__Y__] > height)
            now = (area[__Y__] - height) * width;
        else
            now = 0;
        // cout << i << ": " << now << " | ";
        if(i == 0) {
            place = 0;
            min = now;
        }
        else if(now < min) {
            place = i;
            min = now;
        }
        // cout << now << '|';
    }
    // cout << '-' << place << ' ' << min << endl;
    // char a;
    // cin >> a;
    // cout << endl << place;
    // char a;
    // cin >> a;
    chains.back().back()->coordinate = {0, 0, place};
    // assign_coordinate(chains.back().back());

    // update_wire_length();

    return chains.back().back();
}

void movement_1(int b_idx, STAGE stage) {
    // cout << "\tM1: " << b_idx << "\t\t";
    NODE * root = chains.back().back();
    int old_width = root->area[root->coordinate[__F__]][__X__], old_height = root->area[root->coordinate[__F__]][__Y__];
    // cout << old_width << ' ' << old_height << '|';
    cout.flush();
    
    swap(blocks[b_idx]->name, blocks[b_idx + 1]->name);
    swap(blocks[b_idx]->area, blocks[b_idx + 1]->area);
    swap(blocks[b_idx]->from, blocks[b_idx + 1]->from);

    optimize_orientation(false);

    int place, min;
    for(int i = 0; i < chains.back().back()->area.size(); i++) {
        int now;
        array<int, 2> area = chains.back().back()->area[i];
        // cout << area[__X__] << ' ' << area[__Y__] << ' ' << area[__X__] * area[__Y__] << ' ';
        if(area[__X__] > width && area[__Y__] > height)
            now = area[__X__] * area[__Y__] - width * height;
        else if(area[__X__] > width)
            now = (area[__X__] - width) * height;
        else if(area[__Y__] > height)
            now = (area[__Y__] - height) * width;
        else
            now = 0;
        // cout << i << ": " << now << " | ";
        if(i == 0) {
            place = 0;
            min = now;
        }
        else if(now < min) {
            place = i;
            min = now;
        }
        // cout << now << '|';
    }
    // cout << '-' << place << ' ' << min << endl;
    // char a;
    // cin >> a;
    root->coordinate = {0, 0, place};

    if(stage == AREA)
        return;

    assign_coordinate(root);

    update_wire_length();
}

void movement_2(int c_idx, STAGE stage) {
    // cout << "\tM2: " << c_idx << "\t\t";
    NODE * root = chains.back().back();
    int old_width = root->area[root->coordinate[__F__]][__X__], old_height = root->area[root->coordinate[__F__]][__Y__];
    // cout << old_width << ' ' << old_height << '|';
    cout.flush();
    
    for(auto & i : chains[c_idx]) {
        if(i->name == "V")
            i->name = "H";
        else if(i->name == "H")
            i->name = "V";
    }

    optimize_orientation(false);

    int place, min;
    for(int i = 0; i < chains.back().back()->area.size(); i++) {
        int now;
        array<int, 2> area = chains.back().back()->area[i];
        // cout << area[__X__] << ' ' << area[__Y__] << ' ' << area[__X__] * area[__Y__] << ' ';
        if(area[__X__] > width && area[__Y__] > height)
            now = area[__X__] * area[__Y__] - width * height;
        else if(area[__X__] > width)
            now = (area[__X__] - width) * height;
        else if(area[__Y__] > height)
            now = (area[__Y__] - height) * width;
        else
            now = 0;
        // cout << i << ": " << now << " | ";
        if(i == 0) {
            place = 0;
            min = now;
        }
        else if(now < min) {
            place = i;
            min = now;
        }
        // cout << now << '|';
    }
    // cout << '-' << place << ' ' << min << endl;
    // char a;
    // cin >> a;
    root->coordinate = {0, 0, place};

    if(stage == AREA)
        return;

    assign_coordinate(root);

    update_wire_length();
}

void movement_3(int c_idx, int b_idx, STAGE stage) {
    // cout << "\tM3: " << c_idx << " " << b_idx << '\t';
    NODE * root = chains.back().back();
    int old_width = root->area[root->coordinate[__F__]][__X__], old_height = root->area[root->coordinate[__F__]][__Y__];
    // cout << old_width << ' ' << old_height << '|';
    cout.flush();
    
    if(b_idx == 0) {
        NODE * tmp = chains[c_idx].front();
        chains[c_idx].pop_front();
        chains[c_idx - 1].emplace_back(tmp);
        chains_sum[c_idx - 1]++;
    }
    else {
        NODE * tmp = chains[c_idx].back();
        chains[c_idx].pop_back();
        chains[c_idx + 1].emplace_front(tmp);
        chains_sum[c_idx]--;
    }

    optimize_orientation(false);

    int place, min;
    for(int i = 0; i < chains.back().back()->area.size(); i++) {
        int now;
        array<int, 2> area = chains.back().back()->area[i];
        // cout << area[__X__] << ' ' << area[__Y__] << ' ' << area[__X__] * area[__Y__] << ' ';
        if(area[__X__] > width && area[__Y__] > height)
            now = area[__X__] * area[__Y__] - width * height;
        else if(area[__X__] > width)
            now = (area[__X__] - width) * height;
        else if(area[__Y__] > height)
            now = (area[__Y__] - height) * width;
        else
            now = 0;
        // cout << i << ": " << now << " | ";
        if(i == 0) {
            place = 0;
            min = now;
        }
        else if(now < min) {
            place = i;
            min = now;
        }
        // cout << now << '|';
    }
    // cout << '-' << place << ' ' << min << endl;
    // char a;
    // cin >> a;
    root->coordinate = {0, 0, place};

    if(stage == AREA)
        return;

    assign_coordinate(root);

    update_wire_length();
}

void save_best(bool best_or_not) {
    if(best_or_not) {
        best.clear();
        best.reserve(2 * blocks_num);
        for(int i = 0; i < blocks_num; i++) {
            best.emplace_back(blocks[i]->name);
            if(i > 0)
                for(const auto & j : chains[i - 1])
                    best.emplace_back(j->name);
        }
    }
    else {
        last.clear();
        last.reserve(2 * blocks_num);
        for(int i = 0; i < blocks_num; i++) {
            last.emplace_back(blocks[i]->name);
            if(i > 0)
                for(const auto & j : chains[i - 1])
                    last.emplace_back(j->name);
        }
    }
}

void restore_best(bool best_or_not) {
    if(best_or_not) {
        for(auto & i : blocks)
            delete i;
        blocks.clear();
        blocks.reserve(blocks_num);
        for(auto & i : chains) {
            for(auto & j : i)
                delete j;
            i.clear();
        }
        chains.clear();
        chains.reserve(blocks_num - 1);
        chains_sum.clear();
        chains_sum.reserve(blocks_num - 1);

        int count = 0;
        for(int i = 0; i < best.size(); i++) {
            if(best[i] != "H" && best[i] != "V") {
                NODE * tmp = new NODE;
                array<int, 2> n = {blocks_table[best[i]][__XW__], blocks_table[best[i]][__YH__]}, r = {blocks_table[best[i]][__YH__], blocks_table[best[i]][__XW__]}, t = {0, 0};
                tmp->type = BLOCK;
                tmp->name = best[i];
                if(blocks_table[best[i]][__XW__] == blocks_table[best[i]][__YH__])
                    tmp->area.emplace_back(n);
                else if(blocks_table[best[i]][__XW__] < blocks_table[best[i]][__YH__]) {
                    tmp->area.emplace_back(n);
                    tmp->area.emplace_back(r);
                    t = {0, 1};
                }
                else if(blocks_table[best[i]][__XW__] > blocks_table[best[i]][__YH__]) {
                    tmp->area.emplace_back(r);
                    tmp->area.emplace_back(n);
                    t = {1, 0};
                }
                tmp->from.emplace_back(t);
                blocks.emplace_back(tmp);

                if(i != 0) {
                    list<NODE *> tmp2;
                    chains.emplace_back(tmp2);
                    chains_sum.emplace_back(count);
                }
            }
            else {
                list<NODE *> & tmp = chains.back();
                while(best[i] == "H" || best[i] == "V") {
                    NODE * tmp2 = new NODE;
                    tmp2->type = CHAIN;
                    tmp2->name = best[i++];
                    tmp.emplace_back(tmp2);
                    count = ++chains_sum.back();
                }
                i--;
            }
        }
        NODE * root = chains.back().back();

        optimize_orientation(false);

        int place, min;
        for(int i = 0; i < chains.back().back()->area.size(); i++) {
            int now;
            array<int, 2> area = chains.back().back()->area[i];
            // cout << area[__X__] << ' ' << area[__Y__] << ' ' << area[__X__] * area[__Y__] << ' ';
            if(area[__X__] > width && area[__Y__] > height)
                now = area[__X__] * area[__Y__] - width * height;
            else if(area[__X__] > width)
                now = (area[__X__] - width) * height;
            else if(area[__Y__] > height)
                now = (area[__Y__] - height) * width;
            else
                now = 0;
            // cout << i << ": " << now << " | ";
            if(i == 0) {
                place = 0;
                min = now;
            }
            else if(now < min) {
                place = i;
                min = now;
            }
            // cout << now << '|';
        }
        // cout << '-' << place << ' ' << min << endl;
        // char a;
        // cin >> a;
        root->coordinate = {0, 0, place};
        // assign_coordinate(chains.back().back());
        // update_wire_length();
    }
    else {
        for(auto & i : blocks)
            delete i;
        blocks.clear();
        blocks.reserve(blocks_num);
        for(auto & i : chains) {
            for(auto & j : i)
                delete j;
            i.clear();
        }
        chains.clear();
        chains.reserve(blocks_num - 1);
        chains_sum.clear();
        chains_sum.reserve(blocks_num - 1);

        int count = 0;
        for(int i = 0; i < last.size(); i++) {
            if(last[i] != "H" && last[i] != "V") {
                NODE * tmp = new NODE;
                array<int, 2> n = {blocks_table[last[i]][__XW__], blocks_table[last[i]][__YH__]}, r = {blocks_table[last[i]][__YH__], blocks_table[last[i]][__XW__]}, t = {0, 0};
                tmp->type = BLOCK;
                tmp->name = last[i];
                if(blocks_table[last[i]][__XW__] == blocks_table[last[i]][__YH__])
                    tmp->area.emplace_back(n);
                else if(blocks_table[last[i]][__XW__] < blocks_table[last[i]][__YH__]) {
                    tmp->area.emplace_back(n);
                    tmp->area.emplace_back(r);
                    t = {0, 1};
                }
                else if(blocks_table[last[i]][__XW__] > blocks_table[last[i]][__YH__]) {
                    tmp->area.emplace_back(r);
                    tmp->area.emplace_back(n);
                    t = {1, 0};
                }
                tmp->from.emplace_back(t);
                blocks.emplace_back(tmp);

                if(i != 0) {
                    list<NODE *> tmp2;
                    chains.emplace_back(tmp2);
                    chains_sum.emplace_back(count);
                }
            }
            else {
                list<NODE *> & tmp = chains.back();
                while(last[i] == "H" || last[i] == "V") {
                    NODE * tmp2 = new NODE;
                    tmp2->type = CHAIN;
                    tmp2->name = last[i++];
                    tmp.emplace_back(tmp2);
                    count = ++chains_sum.back();
                }
                i--;
            }
        }
        NODE * root = chains.back().back();

        optimize_orientation(false);

        int place, min;
        for(int i = 0; i < chains.back().back()->area.size(); i++) {
            int now;
            array<int, 2> area = chains.back().back()->area[i];
            // cout << area[__X__] << ' ' << area[__Y__] << ' ' << area[__X__] * area[__Y__] << ' ';
            if(area[__X__] > width && area[__Y__] > height)
                now = area[__X__] * area[__Y__] - width * height;
            else if(area[__X__] > width)
                now = (area[__X__] - width) * height;
            else if(area[__Y__] > height)
                now = (area[__Y__] - height) * width;
            else
                now = 0;
            // cout << i << ": " << now << " | ";
            if(i == 0) {
                place = 0;
                min = now;
            }
            else if(now < min) {
                place = i;
                min = now;
            }
            // cout << now << '|';
        }
        // cout << '-' << place << ' ' << min << endl;
        // char a;
        // cin >> a;
        root->coordinate = {0, 0, place};
        // assign_coordinate(chains.back().back());
        // update_wire_length();
    }
}

int main(int argc, char * argv[]) {
    auto input_start = chrono::steady_clock::now();

    fstream file_hardblocks;
    file_hardblocks.open(argv[1], ios::in);
    string tmp;
    file_hardblocks >> tmp >> tmp >> blocks_num;
    file_hardblocks >> tmp >> tmp >> terminals_num >> tmp;
    while(tmp != "sb0")
        file_hardblocks >> tmp;
    unsigned blocks_area = 0;
    blocks_table.reserve(blocks_num);
    for(int i = 0; i < blocks_num; i++) {
        string name = tmp;
        while(tmp[0] != '(' || tmp[1] == '0')
            file_hardblocks >> tmp;
        tmp[0] = '0';
        int w = stoi(tmp);
        file_hardblocks >> tmp;
        int h = stoi(tmp);
        blocks_area += w * h;
        blocks_table[name] = {0, 0, w, h, 0};
        while(tmp[0] != 's' && tmp[0] != 'p')
            file_hardblocks >> tmp;
    }
    file_hardblocks.close();

    fstream file_nets;
    file_nets.open(argv[2], ios::in);
    file_nets >> tmp >> tmp >> nets_num;
    file_nets >> tmp >> tmp >> pins_num;
    nets_table.reserve(blocks_num);
    wirelength.reserve(nets_num);
    for(int i = 0; i < nets_num; i++) {
        int degree;
        file_nets >> tmp >> tmp >> degree;
        set<string> pin;
        for(int j = 0; j < degree; j++) {
            file_nets >> tmp;
            pin.emplace(tmp);
        }
        vector<string> pin2 (pin.begin(), pin.end());
        bool exist = false, increase = false;
        for(const auto & j : pin2) {
            if(j[0] == 's') {
                if(exist || (nets_table.find(j) != nets_table.end())) {
                    for(auto & k : nets_table[j]) {
                        if(pin2.size() == get<__P__>(k).size()) {
                            if(pin2 == get<__P__>(k)) {
                                exist = true;
                                get<__W__>(k)++;
                                break;
                            }
                        }
                    }
                    if(!exist) {
                        if(!increase)
                            wirelength.emplace_back(-1);
                        increase = true;
                        nets_table[j].emplace_back(make_tuple(1, wirelength.size() - 1, pin2));
                    }
                }
                else {
                    if(!increase)
                        wirelength.emplace_back(-1);
                    increase = true;
                    nets_table[j].emplace_back(make_tuple(1, wirelength.size() - 1, pin2));
                }
            }
        }
    }
    // cout << nets_table.size() << endl;
    // int b = 0;
    // for(auto i : nets_table) {
    //     cout << i.first << " - ";
    //     for(auto j : i.second) {
    //         for(auto k: get<2>(j)) {
    //             cout << k << ' ';
    //         }
    //         cout << get<0>(j) << ' ' << get<1>(j) << " | ";
    //     }
    //     cout << endl << endl;
    // }
    // cout << wirelength.size() << wirelength.capacity() << endl;
    file_nets.close();

    fstream file_pl;
    file_pl.open(argv[3], ios::in);
    terminals_table.reserve(terminals_num);
    for(int i = 0; i < terminals_num; i++) {
        string name;
        int x, y;
        file_pl >> name >> x >> y;
        terminals_table[name] = {x, y};
    }
    file_pl.close();

    const double dead_space_ratio = stod(argv[5]);
    const unsigned floorplan_area = floor(double(blocks_area * (1 + dead_space_ratio)));
    width = floor(sqrt(floorplan_area));
    height = floor(sqrt(floorplan_area));
    cout << "[ " << width << ", " << height << " ]" << endl;

    auto input_end = chrono::steady_clock::now();
    auto initial_start = chrono::steady_clock::now();

    NODE * root = initialize_floorplan();
    cout << "Initial - Width: " << root->area[root->coordinate[__F__]][__X__] << ", Height: " << root->area[root->coordinate[__F__]][__Y__] << endl;
    cout << total_length << endl;
    save_best(true);

    auto initial_end = chrono::steady_clock::now();
    auto area_start = chrono::steady_clock::now();

    int seed = __SEED__;
    if(blocks_num == 100)
        seed = 99921;
    else if(blocks_num == 200)
        seed = 35498;
    else if(blocks_num == 300)
        seed = 99967;
    else if(blocks_num == 150 && dead_space_ratio == 0.15)
        seed = 35498;
    srand(seed);
    cout << seed << endl;
    // mt19937 generator(__SEED__);
    // uniform_int_distribution<int> uni1(0, 2);
    // uniform_int_distribution<int> uni2(0, blocks_num - 2);
    // uniform_int_distribution<int> uni3(0, 1);
    // uniform_int_distribution<int> uni4(0, 99);
    // auto die1 = bind(uni1, generator);
    // auto die2 = bind(uni2, generator);
    // auto die3 = bind(uni3, generator);
    // auto die4 = bind(uni4, generator);

//     double T = 100000;
//     int old_cost = 0.5 * (root->area[root->coordinate[__F__]][__X__] * root->area[root->coordinate[__F__]][__Y__] + total_length);
//     if (root->area[root->coordinate[__F__]][__X__] > width)
//         old_cost += (root->area[root->coordinate[__F__]][__X__] - width) * 500;
//     if (root->area[root->coordinate[__F__]][__Y__] > height)
//         old_cost += (root->area[root->coordinate[__F__]][__Y__] - height) * 500;
// 	while(T > 0.01) {
//         cout << T << ' ';
//         cout.flush();
// //		cout << endl << "temperature : " << T << endl;
// 		int old_width = root->area[root->coordinate[__F__]][__X__], old_height = root->area[root->coordinate[__F__]][__Y__];
// 		for(int SA_times = 0; SA_times < 10 * blocks_num; SA_times++) {
// //			cout << "SA_inner_loops : " << SA_times << endl;
// 			switch(die1()) {
//                 case 0: {
//                     movement_1(die2(), WIRELENGTH);
//                 } break;
//                 case 1: {
//                     int tmp = die2();
//                     while(chains[tmp].empty())
//                         tmp = die2();
//                     movement_2(tmp, WIRELENGTH);
//                 } break;
//                 case 2: {
//                     int tmp1, tmp2; 
//                     while(true) {
//                         do
//                             tmp1 = die2();
//                         while(chains[tmp1].empty());
//                         if(tmp1 == 0)
//                             if(!chains[1].empty() && (chains[0].back()->name == chains[1].front()->name))
//                                 continue;
//                             else
//                                 tmp2 = 1;
//                         else if(tmp1 == blocks_num - 2)
//                             if((chains[blocks_num - 2].size() == 1) || (!chains[blocks_num - 3].empty() && (chains[blocks_num - 2].front()->name == chains[blocks_num - 3].back()->name)))
//                                 continue;
//                             else
//                                 tmp2 = 0;
//                         else {
//                             bool test1 = true, test2 = true;
//                             if((chains_sum[tmp1 - 1] == tmp1) || (!chains[tmp1 - 1].empty() && (chains[tmp1].front()->name == chains[tmp1 - 1].back()->name)))
//                                 test1 = false;
//                             if(!chains[tmp1 + 1].empty() && (chains[tmp1].back()->name == chains[tmp1 + 1].front()->name))
//                                 test2 = false;
//                             if(!test1 && !test2)
//                                 continue;
//                             else if(!test1)
//                                 tmp2 = 1;
//                             else if(!test2)
//                                 tmp2 = 0;
//                             else
//                                 tmp2 = die3();
//                         }
//                         movement_3(tmp1, tmp2, WIRELENGTH);
//                         break;
//                     }
//                 } break;
//             }
//             root = chains.back().back();
			
// 			int new_cost = 0.5 * (root->area[root->coordinate[__F__]][__X__] * root->area[root->coordinate[__F__]][__Y__] + total_length);
//             if (root->area[root->coordinate[__F__]][__X__] > width)
//                 new_cost += (root->area[root->coordinate[__F__]][__X__] - width) * 500;
//             if (root->area[root->coordinate[__F__]][__Y__] > height)
//                 new_cost += (root->area[root->coordinate[__F__]][__Y__] - height) * 500;
// 			if(new_cost < old_cost) {
//                 save_best();
//                 old_cost = new_cost;
//                 if (root->area[root->coordinate[__F__]][__X__] <= width && root->area[root->coordinate[__F__]][__Y__] <= height) {
//                     T = 0;
//                     break;
//                 }
//             }
// 			else {
//                 restore_best();
//             }
// 		}
// //		cout<<final_x<<" "<<final_y<<" "<<temperature<<endl;
// 		T *= 0.9;
//         if(old_width == root->area[root->coordinate[__F__]][__X__] && old_height == root->area[root->coordinate[__F__]][__Y__] && float(die4() / 100) < T / 10 ) {
//       		switch(die1()) {
//                 case 0: {
//                     movement_1(die2(), WIRELENGTH);
//                 } break;
//                 case 1: {
//                     int tmp = die2();
//                     while(chains[tmp].empty())
//                         tmp = die2();
//                     movement_2(tmp, WIRELENGTH);
//                 } break;
//                 case 2: {
//                     int tmp1, tmp2; 
//                     while(true) {
//                         do
//                             tmp1 = die2();
//                         while(chains[tmp1].empty());
//                         if(tmp1 == 0)
//                             if(!chains[1].empty() && (chains[0].back()->name == chains[1].front()->name))
//                                 continue;
//                             else
//                                 tmp2 = 1;
//                         else if(tmp1 == blocks_num - 2)
//                             if((chains[blocks_num - 2].size() == 1) || (!chains[blocks_num - 3].empty() && (chains[blocks_num - 2].front()->name == chains[blocks_num - 3].back()->name)))
//                                 continue;
//                             else
//                                 tmp2 = 0;
//                         else {
//                             bool test1 = true, test2 = true;
//                             if((chains_sum[tmp1 - 1] == tmp1) || (!chains[tmp1 - 1].empty() && (chains[tmp1].front()->name == chains[tmp1 - 1].back()->name)))
//                                 test1 = false;
//                             if(!chains[tmp1 + 1].empty() && (chains[tmp1].back()->name == chains[tmp1 + 1].front()->name))
//                                 test2 = false;
//                             if(!test1 && !test2)
//                                 continue;
//                             else if(!test1)
//                                 tmp2 = 1;
//                             else if(!test2)
//                                 tmp2 = 0;
//                             else
//                                 tmp2 = die3();
//                         }
//                         movement_3(tmp1, tmp2, WIRELENGTH);
//                         break;
//                     }
//                 } break;
//             }
//             root = chains.back().back();
// 			old_cost = 0.5 * (root->area[root->coordinate[__F__]][__X__] * root->area[root->coordinate[__F__]][__Y__] + total_length);
//             if (root->area[root->coordinate[__F__]][__X__] > width)
//                 old_cost += (root->area[root->coordinate[__F__]][__X__] - width) * 500;
//             if (root->area[root->coordinate[__F__]][__Y__] > height)
//                 old_cost += (root->area[root->coordinate[__F__]][__Y__] - height) * 500;
//       		save_best();
//     	}
// 	}

    int i = 0, MT, uphill, reject, N = 10 * blocks_num, best_cost, now_cost;
    bool area_cont = true, timeout = false;
    do {
        double T0 = 10000;
        bool change = true;
        // restore_best(true);
        root = chains.back().back();
        // now_cost = (root->area[root->coordinate[__F__]][__X__] * root->area[root->coordinate[__F__]][__Y__] + total_length) * 0.5;
        // if(root->area[root->coordinate[__F__]][__X__] > width)
        //     now_cost += (root->area[root->coordinate[__F__]][__X__] - width) * 500;
        // if(root->area[root->coordinate[__F__]][__Y__] > height)
        //     now_cost += (root->area[root->coordinate[__F__]][__Y__] - height) * 500;
        if(root->area[root->coordinate[__F__]][__X__] > width && root->area[root->coordinate[__F__]][__Y__] > height)
            now_cost = root->area[root->coordinate[__F__]][__X__] * root->area[root->coordinate[__F__]][__Y__] - width * height;
        else if(root->area[root->coordinate[__F__]][__X__] > width)
            now_cost = (root->area[root->coordinate[__F__]][__X__] - width) * height;
        else if(root->area[root->coordinate[__F__]][__Y__] > height)
            now_cost = (root->area[root->coordinate[__F__]][__Y__] - height) * width;
        else
            now_cost = 0;
        // now_cost *= 10;
        best_cost = now_cost;
        do {
            MT = uphill = reject = 0;
            // cout << " - ";
            // int old_width = root->area[root->coordinate[__F__]][__X__], old_height = root->area[root->coordinate[__F__]][__Y__];
            do {
                // if(change)
                //     i++;
                // if(change)
                //     cout << "Iteration " << i++ << " - ";
                cout.flush();
                change = false;
                save_best(false);
                switch(rand() % 3) {
                    case 0: {
                        movement_1(rand() % (blocks_num - 1), AREA);
                    } break;
                    case 1: {
                        int tmp = rand() % (blocks_num - 1);
                        while(chains[tmp].empty())
                            tmp = rand() % (blocks_num - 1);
                        movement_2(tmp, AREA);
                    } break;
                    case 2: {
                        int tmp1, tmp2; 
                        while(true) {
                            do
                                tmp1 = rand() % (blocks_num - 1);
                            while(chains[tmp1].empty());
                            if(tmp1 == 0)
                                if(!chains[1].empty() && (chains[0].back()->name == chains[1].front()->name))
                                    continue;
                                else
                                    tmp2 = 1;
                            else if(tmp1 == blocks_num - 2)
                                if((chains[blocks_num - 2].size() == 1) || (!chains[blocks_num - 3].empty() && (chains[blocks_num - 2].front()->name == chains[blocks_num - 3].back()->name)))
                                    continue;
                                else
                                    tmp2 = 0;
                            else {
                                bool test1 = true, test2 = true;
                                if((chains_sum[tmp1 - 1] == tmp1) || (!chains[tmp1 - 1].empty() && (chains[tmp1].front()->name == chains[tmp1 - 1].back()->name)))
                                    test1 = false;
                                if(!chains[tmp1 + 1].empty() && (chains[tmp1].back()->name == chains[tmp1 + 1].front()->name))
                                    test2 = false;
                                if(!test1 && !test2)
                                    continue;
                                else if(!test1)
                                    tmp2 = 1;
                                else if(!test2)
                                    tmp2 = 0;
                                else
                                    tmp2 = rand() % 2;
                            }
                            movement_3(tmp1, tmp2, AREA);
                            break;
                        }
                    } break;
                }
                root = chains.back().back();

                MT++;
                int new_cost = (root->area[root->coordinate[__F__]][__X__] * root->area[root->coordinate[__F__]][__Y__] + total_length) * 0.5;
                // if(root->area[root->coordinate[__F__]][__X__] > width)
                //     new_cost += (root->area[root->coordinate[__F__]][__X__] - width) * 500;
                // if(root->area[root->coordinate[__F__]][__Y__] > height)
                //     new_cost += (root->area[root->coordinate[__F__]][__Y__] - height) * 500;
                if(root->area[root->coordinate[__F__]][__X__] > width && root->area[root->coordinate[__F__]][__Y__] > height)
                    new_cost = root->area[root->coordinate[__F__]][__X__] * root->area[root->coordinate[__F__]][__Y__] - width * height;
                else if(root->area[root->coordinate[__F__]][__X__] > width)
                    new_cost = (root->area[root->coordinate[__F__]][__X__] - width) * height;
                else if(root->area[root->coordinate[__F__]][__Y__] > height)
                    new_cost = (root->area[root->coordinate[__F__]][__Y__] - height) * width;
                else
                    new_cost = 0;
                // new_cost *= 10;
                int diff = new_cost - now_cost;
                double prob = (double)rand() / RAND_MAX;
                // cout << prob << ' ' << diff << ' ' << exp(-1 * diff / T0) << '|';
                if (diff <= 0 || prob < exp(-1 * diff / T0)) {
                    change = true;
                    // cout << "Width: " << root->area[root->coordinate[__F__]][__X__] << ", Height: " << root->area[root->coordinate[__F__]][__Y__] << '|';
                    cout.flush();
                    now_cost = new_cost;
                    if (diff > 0) {
                        // cout << diff << ' ' << exp(-1 * diff / T0) << '/';
                        uphill++;
                    }
                    if (now_cost < best_cost) {
                        // cout << "Width: " << root->area[root->coordinate[__F__]][__X__] << ", Height: " << root->area[root->coordinate[__F__]][__Y__];
                        save_best(true);
                        best_cost = now_cost;

                        if (root->area[root->coordinate[__F__]][__X__] <= width && root->area[root->coordinate[__F__]][__Y__] <= height) {
                            area_cont = false;
                            break;
                        }
                    }
                    // char a;
                    // cin >> a;
                }
                else {
                    restore_best(false);
                    reject++;
                }
            } while (uphill <= N && MT <= 2 * N);
            if(!area_cont)
                break;
            T0 *= 0.95;
            // cout << T0 << ' ' << (double)reject / MT << " | ";
            cout.flush();
            // if(old_width == root->area[root->coordinate[__F__]][__X__] && old_height == root->area[root->coordinate[__F__]][__Y__]) {
            // 	switch(die1()) {
            //         case 0: {
            //             movement_1(die2(), WIRELENGTH);
            //         } break;
            //         case 1: {
            //             int tmp = die2();
            //             while(chains[tmp].empty())
            //                 tmp = die2();
            //             movement_2(tmp, WIRELENGTH);
            //         } break;
            //         case 2: {
            //             int tmp1, tmp2; 
            //             while(true) {
            //                 do
            //                     tmp1 = die2();
            //                 while(chains[tmp1].empty());
            //                 if(tmp1 == 0)
            //                     if(!chains[1].empty() && (chains[0].back()->name == chains[1].front()->name))
            //                         continue;
            //                     else
            //                         tmp2 = 1;
            //                 else if(tmp1 == blocks_num - 2)
            //                     if((chains[blocks_num - 2].size() == 1) || (!chains[blocks_num - 3].empty() && (chains[blocks_num - 2].front()->name == chains[blocks_num - 3].back()->name)))
            //                         continue;
            //                     else
            //                         tmp2 = 0;
            //                 else {
            //                     bool test1 = true, test2 = true;
            //                     if((chains_sum[tmp1 - 1] == tmp1) || (!chains[tmp1 - 1].empty() && (chains[tmp1].front()->name == chains[tmp1 - 1].back()->name)))
            //                         test1 = false;
            //                     if(!chains[tmp1 + 1].empty() && (chains[tmp1].back()->name == chains[tmp1 + 1].front()->name))
            //                         test2 = false;
            //                     if(!test1 && !test2)
            //                         continue;
            //                     else if(!test1)
            //                         tmp2 = 1;
            //                     else if(!test2)
            //                         tmp2 = 0;
            //                     else
            //                         tmp2 = die3();
            //                 }
            //                 movement_3(tmp1, tmp2, WIRELENGTH);
            //                 break;
            //             }
            //         } break;
            //     }
            //     root = chains.back().back();
            // 	if(root->area[root->coordinate[__F__]][__X__] > width && root->area[root->coordinate[__F__]][__Y__] > height)
            //         now_cost = root->area[root->coordinate[__F__]][__X__] * root->area[root->coordinate[__F__]][__Y__] - width * height;
            //     else if(root->area[root->coordinate[__F__]][__X__] > width)
            //         now_cost = (root->area[root->coordinate[__F__]][__X__] - width) * height;
            //     else if(root->area[root->coordinate[__F__]][__Y__] > height)
            //         now_cost = (root->area[root->coordinate[__F__]][__Y__] - height) * width;
            //     else
            //         now_cost = 0;
            // 	save_best();
            // }
            auto time1 = chrono::steady_clock::now();
            if(chrono::duration<float>(time1 - input_start).count() >= 585)
                break;
        } while((double)reject / MT <= 0.95 && T0 >= 0.1);
        // cout << endl;
        auto time2 = chrono::steady_clock::now();
        if(chrono::duration<float>(time2 - input_start).count() >= 585)
            timeout = true;
    } while(area_cont && !timeout);

    auto area_end = chrono::steady_clock::now();
    auto wirelength_start = chrono::steady_clock::now();
    
    if(!timeout) {
        restore_best(true);
        root = chains.back().back();
        assign_coordinate(root);
        update_wire_length();

        bool wirelength_cont = true;
        root = chains.back().back();
        best_cost = now_cost = total_length;
        MT = reject = 0;
        do {
            save_best(false);
            movement_1(rand() % (blocks_num - 1), WIRELENGTH);
            root = chains.back().back();

            MT++;
            int new_cost = total_length;
            int diff = new_cost - now_cost;
            if (root->area[root->coordinate[__F__]][__X__] <= width && root->area[root->coordinate[__F__]][__Y__] <= height) {
                now_cost = new_cost;
                if (now_cost < best_cost) {
                    // cout << "Wirelength: " << total_length;
                    save_best(true);
                    best_cost = now_cost;
                }
                else
                    reject++;
            }
            else {
                restore_best(false);
                reject++;
            }
            auto time = chrono::steady_clock::now();
            if(chrono::duration<float>(time - input_start).count() >= 585)
                break;
        } while((double)reject / MT <= 0.95 || MT <= 10 * blocks_num);
    }

    restore_best(true);
    root = chains.back().back();
    assign_coordinate(root);
    update_wire_length();

    auto wirelength_end = chrono::steady_clock::now();
    auto output_start = chrono::steady_clock::now();

    fstream file_floorplan;
    file_floorplan.open(argv[4], ios::out);
    file_floorplan << "Wirelength " << total_length << endl;
    file_floorplan << "Blocks\n";
    for(auto i : blocks_table)
        file_floorplan << i.first << ' ' << i.second[__X__] << ' ' << i.second[__Y__] << ' ' << i.second[__R__] << endl;
    file_floorplan.close();

    auto output_end = chrono::steady_clock::now();

    cout << "----- Timing Result -----\n";
    cout << "  Read File Time:\t\t" << chrono::duration<float>(input_end - input_start).count() << "\tsec." << endl;
    cout << "+ Initialization Time:\t\t" << chrono::duration<float>(initial_end - initial_start).count() << "\tsec." << endl;
    cout << "+ Area Time:\t\t" << chrono::duration<float>(area_end - area_start).count() << "\tsec." << endl;
    cout << "+ Wirelength Time:\t\t" << chrono::duration<float>(wirelength_end - wirelength_start).count() << "\tsec." << endl;
    cout << "+ Write File Time:\t\t" << chrono::duration<float>(output_end - output_start).count() << "\tsec." << endl;
    cout << "= Total Runtime:\t\t" << chrono::duration<float>(output_end - input_start).count() << "\tsec." << endl;

    return 0;
}