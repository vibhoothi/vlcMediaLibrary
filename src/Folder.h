/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#pragma once

#include "database/DatabaseHelpers.h"
#include "IFolder.h"

#include <sqlite3.h>

class Folder;

namespace fs
{
    class IDirectory;
}

namespace policy
{
struct FolderTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static unsigned int Folder::*const PrimaryKey;
};

}

class Folder : public IFolder, public DatabaseHelpers<Folder, policy::FolderTable>
{
public:
    Folder( DBConnection dbConnection, sqlite::Row& row );
    Folder( const std::string& path, time_t lastModificationDate, bool isRemovable, unsigned int parent );

    static bool createTable( DBConnection connection );
    static std::shared_ptr<Folder> create( DBConnection connection, const std::string& path, time_t lastModificationDate, bool isRemovable, unsigned int parentId );

    static std::shared_ptr<Folder> fromPath( DBConnection conn, const std::string& path );

    virtual unsigned int id() const override;
    virtual const std::string& path() const override;
    virtual std::vector<MediaPtr> files() override;
    virtual std::vector<FolderPtr> folders() override;
    virtual FolderPtr parent() override;
    virtual unsigned int lastModificationDate() override;
    bool setLastModificationDate(unsigned int lastModificationDate);
    bool isRemovable();

private:
    DBConnection m_dbConection;

    unsigned int m_id;
    std::string m_path;
    unsigned int m_parent;
    unsigned int m_lastModificationDate;
    bool m_isRemovable;

    friend struct policy::FolderTable;
};
