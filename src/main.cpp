#include <iostream>
#include <unordered_map>
#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <fstream>
#include <regex>

using namespace std;
using namespace chrono;

class KVStore {

private:

    unordered_map<string,string> store;
    unordered_map<string,steady_clock::time_point> expiry;

    mutex mtx;

    size_t expired_cleaned = 0;

    bool isExpired(const string &key){

        if(expiry.count(key)){

            if(steady_clock::now() >= expiry[key]){

                store.erase(key);
                expiry.erase(key);
                expired_cleaned++;

                return true;
            }
        }

        return false;
    }

public:

    string set(string key,string value,int ex=-1){

        lock_guard<mutex> lock(mtx);

        store[key] = value;

        if(ex>0)
            expiry[key] = steady_clock::now() + seconds(ex);
        else
            expiry.erase(key);

        return "OK";
    }

    string get(string key){

        lock_guard<mutex> lock(mtx);

        if(!store.count(key))
            return "(nil)";

        if(isExpired(key))
            return "(nil)";

        return store[key];
    }

    string del(string key){

        lock_guard<mutex> lock(mtx);

        if(!store.count(key))
            return "(nil)";

        store.erase(key);
        expiry.erase(key);

        return "OK";
    }

    string ttl(string key){

        lock_guard<mutex> lock(mtx);

        if(!store.count(key))
            return "-2";

        if(isExpired(key))
            return "-2";

        if(!expiry.count(key))
            return "-1";

        auto remaining = duration_cast<seconds>(expiry[key] - steady_clock::now()).count();

        if(remaining < 0)
            return "-2";

        return to_string(remaining);
    }

    string keys(string pattern){

        lock_guard<mutex> lock(mtx);

        string regex_pattern = regex_replace(pattern, regex("\\*"), ".*");
        regex r(regex_pattern);

        string result;

        for(auto &p : store){

            if(isExpired(p.first))
                continue;

            if(regex_match(p.first,r))
                result += p.first + "\n";
        }

        if(result.empty())
            return "(empty)";

        return result;
    }

    void save(){

        lock_guard<mutex> lock(mtx);

        ofstream file("snapshot.json");

        file<<"{\n";

        bool first=true;

        for(auto &p : store){

            if(isExpired(p.first))
                continue;

            if(!first)
                file<<",\n";

            file<<"\""<<p.first<<"\":\""<<p.second<<"\"";

            first=false;
        }

        file<<"\n}";
    }

    void load(){

        lock_guard<mutex> lock(mtx);

        ifstream file("snapshot.json");

        if(!file)
            return;

        string content((istreambuf_iterator<char>(file)),
                       istreambuf_iterator<char>());

        regex pair_regex("\"([^\"]+)\":\"([^\"]+)\"");

        auto begin = sregex_iterator(content.begin(),content.end(),pair_regex);
        auto end = sregex_iterator();

        for(auto i=begin;i!=end;i++){

            string key = (*i)[1];
            string value = (*i)[2];

            store[key] = value;
        }
    }

    string stats(){

        lock_guard<mutex> lock(mtx);

        size_t memory = 0;

        for(auto &p:store)
            memory += p.first.size() + p.second.size();

        stringstream ss;

        ss<<"Total keys: "<<store.size()<<"\n";
        ss<<"Expired keys cleaned: "<<expired_cleaned<<"\n";
        ss<<"Memory usage estimate: "<<memory<<" bytes";

        return ss.str();
    }

    void cleanupLoop(){

        while(true){

            this_thread::sleep_for(seconds(1));

            lock_guard<mutex> lock(mtx);

            for(auto it=expiry.begin(); it!=expiry.end();){

                if(steady_clock::now() >= it->second){

                    store.erase(it->first);
                    it = expiry.erase(it);

                    expired_cleaned++;

                }else{
                    ++it;
                }
            }
        }
    }
};

vector<string> split(string line){

    stringstream ss(line);

    vector<string> tokens;

    string word;

    while(ss>>word)
        tokens.push_back(word);

    return tokens;
}

int main(){

    KVStore kv;

    thread cleaner(&KVStore::cleanupLoop,&kv);
    cleaner.detach();

    string line;

    while(getline(cin,line)){

        vector<string> cmd = split(line);

        if(cmd.empty())
            continue;

        string op = cmd[0];

        if(op=="SET"){

            if(cmd.size()>=3){

                if(cmd.size()==5 && cmd[3]=="EX")
                    cout<<kv.set(cmd[1],cmd[2],stoi(cmd[4]))<<endl;
                else
                    cout<<kv.set(cmd[1],cmd[2])<<endl;
            }
        }

        else if(op=="GET")
            cout<<kv.get(cmd[1])<<endl;

        else if(op=="DEL")
            cout<<kv.del(cmd[1])<<endl;

        else if(op=="TTL")
            cout<<kv.ttl(cmd[1])<<endl;

        else if(op=="KEYS")
            cout<<kv.keys(cmd[1])<<endl;

        else if(op=="SAVE"){

            kv.save();
            cout<<"OK"<<endl;
        }

        else if(op=="LOAD"){

            kv.load();
            cout<<"OK"<<endl;
        }

        else if(op=="STATS")
            cout<<kv.stats()<<endl;
    }
}