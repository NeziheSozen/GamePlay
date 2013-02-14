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

const string& IdStore::getId(const string& name, const string& uniqueId)
{
    string checkedName(name);
    if (checkedName.empty())
    {
        checkedName.assign("Node");
    }

    pair<UniqueIdToNameMultiMap::const_iterator,UniqueIdToNameMultiMap::const_iterator> range = _uniqueIdToName.equal_range(checkedName);

    if (range.first == range.second)
    {
        // name not yet used
        pair<string, string> ids;
        ids.first = uniqueId;
        ids.second = checkedName;
        
		UniqueIdToNameMultiMap::iterator it;
		it = _uniqueIdToName.insert(pair<string,pair<string,string> >(checkedName,ids));
		
		return it->second.second;
    }

    UniqueIdToNameMultiMap::const_iterator it;
    int count = 0;
    for (it = range.first; it != range.second; it++)
    {
        if (it->second.first == uniqueId) {
            // found uniqueId -> return stored id
            return it->second.second;
        }
        count++;
    }

    // uniqueId unkown, create new id and add it to the list
    pair<string, string> ids;
    ids.first = uniqueId;
    stringstream ss;
    ss << checkedName << "_" << count;
    ids.second = ss.str();
	it = _uniqueIdToName.insert(pair<string,pair<string,string> >(checkedName,ids));

    return ids.second;
}
