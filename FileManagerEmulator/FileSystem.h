#pragma once

#include "LeakDetect.h"

#include <string>
#include <memory>
#include <functional>
#include <iosfwd>

namespace FileSystem
{
    struct Item;
    struct Composite;
    struct Link;
    struct Linkable;

    typedef std::shared_ptr<Item> ItemPtr;
    typedef std::weak_ptr<Item> ItemWeakPtr;
    typedef std::string Path;
    typedef std::string Name;

    // bool (ItemPtr& item, size_t index, size_t size)
    typedef std::function<bool(ItemPtr&, size_t, size_t)> IterateFunction;

    // bool (const ItemPtr& item, size_t index, size_t size)
    typedef std::function<bool(const ItemPtr&, size_t, size_t)> ConstIterateFunction;

    enum class ItemType
    {
        eDrive,
        eDirectory,
        eFile,
        eHardLink,
        eDynamicLink
    };

    struct Item
    {
        static ItemPtr create(ItemType type);

        virtual ItemType type() const = 0;

        virtual Name name() const = 0;

        virtual ItemPtr self() = 0;

        virtual ItemWeakPtr parent() const = 0;

        virtual ItemPtr copy() const = 0;

        virtual bool deletable() const = 0;

        virtual Path fullPath() const = 0;

        virtual void setName(const Name& name) = 0;

        virtual void setParent(const ItemWeakPtr& parent) = 0;

        virtual Composite* asComposite() = 0;
        virtual const Composite* asComposite() const = 0;

        virtual Linkable* asLinkable() = 0;
        virtual const Linkable* asLinkable() const = 0;

        virtual Link* asLink() = 0;

        virtual ~Item() {}
    };

    struct Composite
    {
        virtual bool empty() const = 0;

        virtual bool childrenDeletable() const = 0;

        virtual bool iterate(ConstIterateFunction func, bool sorted) const = 0;

        virtual bool iterate(IterateFunction func, bool sorted) = 0;

        virtual bool addChild(const ItemPtr& item) = 0;

        virtual ItemPtr removeChild(const Item& item) = 0;

        virtual Item* findChild(const Name& name) const = 0;

        virtual void removeChildren() = 0;

        virtual ~Composite() {}
    };

    struct Link
    {
        virtual bool linkTo(const ItemPtr& linked) = 0;

        virtual ~Link() {}
    };

    struct Linkable
    {
        virtual bool linkedHard() const = 0;

        virtual void addLink(const ItemWeakPtr& link, bool hard) = 0;

        virtual void removeLink(const Item& link, bool hard) = 0;

        virtual ~Linkable() {}
    };
}
