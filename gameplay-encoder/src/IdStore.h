//
//  IdStore.h
//  gameplay-encoder
//
//  Created by Wolfgang Damm on 2/9/13.
//
//

#ifndef __gameplay_encoder__IdStore__
#define __gameplay_encoder__IdStore__

#include <string>
#include <map>

typedef std::multimap<std::string, std::pair<std::string, std::string> > UniqueIdToNameMultiMap;

/**
 * Stores a list of used ids. Allows to query the id to a node's name.
 */
class IdStore {
public:
    IdStore();
    virtual ~IdStore();

    const std::string& getId(const char* name, const char* uniqueId);

    /**
     * Retrieves the id (unique) for a given name. If all node's have unique names, the
     * id returned is the name. The uniqueId is used to detect name conflicts. Once such
     * a conflict is encountered a number is appended to the name.
     */
    const std::string& getId(const std::string& name, const std::string& uniqueId);

private:
    UniqueIdToNameMultiMap _uniqueIdToName;
};


#endif /* defined(__gameplay_encoder__IdStore__) */
