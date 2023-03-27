/*
 * Fooyin
 * Copyright 2022-2023, Luke Taylor <LukeT1@proton.me>
 *
 * Fooyin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Fooyin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fooyin.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <core/library/sorting/sortorder.h>
#include <core/typedefs.h>

#include <QObject>

namespace Fy::Filters {
Q_NAMESPACE
enum class FilterType
{
    Genre = 0,
    Year,
    AlbumArtist,
    Artist,
    Album,
};
Q_ENUM_NS(FilterType)

struct LibraryFilter
{
    int index;
    Filters::FilterType type;
    Core::Library::SortOrder sortOrder;
};
using LibraryFilters = std::vector<LibraryFilter>;

struct FilterEntry
{
    int id;
    QString name;

    bool operator<(const FilterEntry& other) const
    {
        return std::tuple(id, name) < std::tuple(other.id, other.name);
    }

    bool operator==(const FilterEntry& other) const
    {
        return std::tuple(id, name) == std::tuple(other.id, other.name);
    }

    bool operator==(int otherId) const
    {
        return id == otherId;
    }

    bool operator==(const QString& otherName) const
    {
        return name == otherName;
    }
};

using FilterEntries  = std::vector<FilterEntry>;
using FilterEntrySet = std::set<FilterEntry>;
using ActiveFilters  = std::unordered_map<Filters::FilterType, FilterEntrySet>;
} // namespace Fy::Filters
