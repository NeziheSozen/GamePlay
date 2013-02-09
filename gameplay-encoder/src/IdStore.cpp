//
//  IdStore.cpp
//  gameplay-encoder
//
//  Created by Wolfgang Damm on 2/9/13.
//
//

#include "IdStore.h"

#include <sstream>

using namespace std;

IdStore::IdStore()
{
}

IdStore::~IdStore()
{

}

string IdStore::getId(const string& name, const string& uniqueId)
{
    pair<UniqueIdToNameMultiMap::const_iterator,UniqueIdToNameMultiMap::const_iterator> range = _uniqueIdToName.equal_range(name);

    if (range.first == range.second)
    {
        // name not yet used
        pair<string, string> ids;
        ids.first = uniqueId;
        ids.second = name;
        _uniqueIdToName.insert(pair<string,pair<string,string> >(name,ids));

        return name;
    }

    UniqueIdToNameMultiMap::const_iterator it;
    int count = 0;
    for (it = range.first; it != range.second; it++)
    {
        if (it->first == uniqueId) {
            // found uniqueId -> return stored id
            return it->second.second;
        }
        count++;
    }

    // uniqueId unkown, create new id and add it to the list
    pair<string, string> ids;
    ids.first = uniqueId;
    stringstream ss;
    ss << name << "_" << count;
    ids.second = ss.str();
    _uniqueIdToName.insert(pair<string,pair<string,string> >(name,ids));

    return ids.second;
}
