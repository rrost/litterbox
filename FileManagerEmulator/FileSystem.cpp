#include "FileSystem.h"

#include <algorithm>
#include <cassert>
#include <vector>
#include <unordered_map>

#include "Utils.h"

namespace FileSystem
{

    ///////////////////////////////////////////////////////////////////////////////////////////////
    class ItemBase:
        public Item,
        public std::enable_shared_from_this<ItemBase>
    {
        ItemWeakPtr parent_;

        ItemBase& operator=(const ItemBase&) = delete;

    protected:

        ItemBase() {}

        ItemBase(const ItemBase&): parent_() {}

        virtual ~ItemBase() {}

        virtual ItemWeakPtr parent() const override
        {
            return parent_;
        }

        virtual ItemPtr self() override
        {
            return shared_from_this();
        }

        virtual bool deletable() const override
        {
            return true;
        }

        virtual Path fullPath() const override
        {
            if(parent_.expired()) return name(); // We are the root

            const auto daddy = parent_.lock();
            return daddy->fullPath() + Utils::DirectoryDelimiter + name();
        }

        virtual void setParent(const ItemWeakPtr& parent) override
        {
            parent_ = parent;
        }

        virtual Composite* asComposite() override
        {
            return nullptr;
        }

        virtual Linkable* asLinkable() override
        {
            return nullptr;
        }

        virtual const Composite* asComposite() const override
        {
            return nullptr;
        }

        virtual const Linkable* asLinkable() const override
        {
            return nullptr;
        }

        virtual Link* asLink() override
        {
            return nullptr;
        }

    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    class CompositeBase: public Composite
    {
        typedef std::vector<ItemPtr> Children;
        Children children_;

        CompositeBase& operator=(const CompositeBase&) = delete;

    protected:

        CompositeBase() {}

        CompositeBase(const CompositeBase&) = default;

        virtual ~CompositeBase() {}

        virtual bool empty() const override
        {
            return children_.empty();
        }

        template <class ThisType, typename IterateFunc>
        static bool iterateChildren(ThisType& self, const IterateFunc& func, bool sorted)
        {
            auto& items = self.children_;
            const size_t size = items.size();
            std::vector<size_t> sortedIndex(size);

            size_t count = 0;
            for(auto& idx : sortedIndex) idx = count++;
            assert(sortedIndex.size() == size);
            assert(sortedIndex.empty() || sortedIndex.back() == size - 1);

            if(sorted)
            {
                const auto compareItems = [&items](size_t lhs, size_t rhs)
                {
                    return items[lhs]->name() < items[rhs]->name();
                };

                std::sort(sortedIndex.begin(), sortedIndex.end(), compareItems);
            }

            size_t index = 0;
            for(const auto i : sortedIndex)
            {
                if(!func(items[i], index++, size)) return false;
            }

            return true;
        }

        virtual bool iterate(ConstIterateFunction func, bool sorted) const override
        {
            return iterateChildren(*this, func, sorted);
        }

        virtual bool iterate(IterateFunction func, bool sorted) override
        {
            return iterateChildren(*this, func, sorted);
        }

        virtual bool addChild(const ItemPtr& item) override
        {
            const auto found =  findChild(item->name());
            if(found) return false;

            children_.push_back(item);
            return true;
        }

        virtual ItemPtr removeChild(const Item& item) override
        {
            const auto found = std::find_if(children_.cbegin(), children_.cend(),
                [&item](const ItemPtr& p) { return p.get() == &item; });

            if(found == children_.cend()) return ItemPtr();

            const auto removed(*found);
            children_.erase(found);
            return removed;
        }

        virtual Item* findChild(const Name& name) const override
        {
            for(const auto& p : children_)
            {
                if(Utils::equalNoCase(name, p->name())) return p.get();
            }

            return nullptr;
        }

        virtual void removeChildren() override
        {
            for(auto& p : children_)
            {
                if(p->asComposite())
                {
                    p->asComposite()->removeChildren();
                    if(!p->asComposite()->empty()) continue;
                }
                if(p->deletable()) p.reset();
            }

            children_.erase(
                std::remove(children_.begin(), children_.end(), ItemPtr()), children_.end());
        }

        virtual bool childrenDeletable() const override
        {
            for(const auto& p : children_)
            {
                if(!p->deletable()) return false;
                if(p->asComposite() && !p->asComposite()->childrenDeletable()) return false;
            }
            return true;
        }

    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    class LinkableBase: public Linkable
    {
        typedef std::unordered_map<const Item*, ItemWeakPtr> Links;
        Links hardLinks_;
        Links dynamicLinks_;

        LinkableBase& operator=(const LinkableBase&) = delete;

    protected:

        LinkableBase() {}

        LinkableBase(const LinkableBase&): hardLinks_(), dynamicLinks_() {}

        virtual ~LinkableBase()
        {
            for(const auto& w : dynamicLinks_)
            {
                if(w.second.expired()) continue;
                const auto link = w.second.lock();

                if(link->parent().expired()) continue;
                const auto parent = link->parent().lock();

                assert(parent);
                auto dir = parent->asComposite();

                assert(dir);
                dir->removeChild(*link);
            }
        }

        virtual void addLink(const ItemWeakPtr& link, bool hard) override
        {
            assert(!link.expired());
            const auto newLink = link.lock();

            auto& links = hard ? hardLinks_ : dynamicLinks_;

            const auto found = links.find(newLink.get());
            if(found != links.cend()) return;

            links[newLink.get()] = link;
        }

        virtual void removeLink(const Item& link, bool hard) override
        {
            auto& links = hard ? hardLinks_ : dynamicLinks_;
            links.erase(&link);
        }

        virtual bool linkedHard() const override
        {
            return !hardLinks_.empty();
        }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    class ItemLink:
        public ItemBase,
        public Link
    {
        const bool hard_;
        ItemWeakPtr linked_;

        virtual ItemType type() const override
        {
            return hard_ ? ItemType::eHardLink : ItemType::eDynamicLink;
        }

        virtual Name name() const override
        {
            const auto item = linked_.lock();
            const Name prefix = hard_ ? "hlink" : "dlink";
            return prefix + "[" + (item ? item->fullPath() : "<none>") + "]";
        }

        virtual ItemPtr copy() const override
        {
            const ItemPtr clone(new ItemLink(*this));
            clone->setParent(ItemWeakPtr());
            return clone;
        }

        virtual void setName(const Name&) override
        {
            assert(false);
        }

        virtual bool linkTo(const ItemPtr& linked) override
        {
            assert(linked);
            if(!linked->asLinkable()) return false;

            linked->asLinkable()->addLink(shared_from_this(), hard_);
            linked_ = linked;
            return true;
        }

        virtual Link* asLink() override
        {
            return this;
        }

    public:

        ItemLink(bool hard): hard_(hard) {}

        ItemLink(const ItemLink&) = default;

        virtual ~ItemLink()
        {
            const auto item = linked_.lock();
            if(item)
            {
                assert(item->asLinkable());
                item->asLinkable()->removeLink(*this, hard_);
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    class File:
        public ItemBase,
        public LinkableBase
    {
        Name name_;

        virtual ItemType type() const override
        {
            return ItemType::eFile;
        }

        virtual Name name() const override
        {
            return name_;
        }

        virtual ItemPtr copy() const override
        {
            return ItemPtr(new File(*this));
        }

        virtual bool deletable() const override
        {
            return !linkedHard();
        }

        virtual void setName(const Name& name) override
        {
            assert(Utils::validFileName(name));

            name_ = name;
            Utils::toLowerCase(name_);
        }

        virtual Linkable* asLinkable() override
        {
            return this;
        }

        virtual const Linkable* asLinkable() const override
        {
            return this;
        }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    class Directory:
        public ItemBase,
        public CompositeBase,
        public LinkableBase
    {
        Name name_;

        virtual ItemType type() const override
        {
            return ItemType::eDirectory;
        }

        virtual ItemPtr copy() const override
        {
            const ItemPtr clone(new Directory(*this));

            const auto copyItems = [&clone](ItemPtr& item, size_t, size_t)
            {
                const auto itemCopy = item->copy();
                if(!itemCopy) return false;
                itemCopy->setParent(clone);

                item = itemCopy;
                return true;
            };

            if(!clone->asComposite()->iterate(copyItems, false)) return ItemPtr();

            return clone;
        }

        virtual bool deletable() const override
        {
            // Directory deletable if:
            //  - not referenced by hard links
            //  - not referenced as current directory
            return !linkedHard() && (shared_from_this().use_count() == 2);
        }

        virtual Name name() const override
        {
            return name_;
        }

        virtual void setName(const Name& name) override
        {
            assert(type() == ItemType::eDirectory ?
                Utils::validDirectoryName(name) :
                Utils::validDriveName(name));

            name_ = name;
            Utils::toUpperCase(name_);
        }

        virtual bool addChild(const ItemPtr& item) override
        {
            if(CompositeBase::addChild(item))
            {
                item->setParent(shared_from_this());
                return true;
            }

            return false;
        }

        virtual Composite* asComposite() override
        {
            return this;
        }

        virtual const Composite* asComposite() const override
        {
            return this;
        }

        virtual Linkable* asLinkable() override
        {
            return this;
        }

        virtual const Linkable* asLinkable() const override
        {
            return this;
        }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    class Drive: public Directory
    {
        virtual ItemType type() const override
        {
            return ItemType::eDrive;
        }

        virtual bool deletable() const override
        {
            return false;
        }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ItemPtr Item::create(ItemType type)
    {
        switch(type)
        {
        case ItemType::eDrive:       return std::make_shared<Drive>();
        case ItemType::eDirectory:   return std::make_shared<Directory>();
        case ItemType::eFile:        return std::make_shared<File>();
        case ItemType::eHardLink:    return std::make_shared<ItemLink>(true);
        case ItemType::eDynamicLink: return std::make_shared<ItemLink>(false);
        }

        assert(false);
        return ItemPtr();
    }

}
