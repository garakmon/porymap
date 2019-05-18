#ifndef GUARD_PORYMAP_INTERFACE_H
#define GUARD_PORYMAP_INTERFACE_H

#include <QtPlugin>

// interface for extending porymap's ui and main window (and project?)
class PorymapInterface
{
public:
    //
    virtual ~PorymapInterface() {}
};

// for editing porymap's i/o functions if you change things in files
class IoInterface
{
public:
    //
    virtual ~IoInterface() {}
};

// provides an interface for plugins to add support 
// for new types of events to porymap 
class EventInterface
{
public:
    //
    virtual ~EventInterface() {}

    virtual QStringList events() const = 0;
};

// for making major modifications to your game's region map via plugin
// eg. view cursor, larger map, multiple region maps, show wild mons?
class RegionMapInterface
{
public:
    //
    virtual ~RegionMapInterface() {}
};

#endif // GUARD_PORYMAP_INTERFACE_H
