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

#include "Genre.h"

#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"

namespace policy
{
const std::string GenreTable::Name = "Genre";
const std::string GenreTable::PrimaryKeyColumn = "id_genre";
unsigned int Genre::* const GenreTable::PrimaryKey = &Genre::m_id;
}

Genre::Genre( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
{
    row >> m_id
        >> m_name;
}

Genre::Genre( MediaLibraryPtr ml, const std::string& name )
    : m_ml( ml )
    , m_name( name )
{
}

unsigned int Genre::id() const
{
    return m_id;
}

const std::string& Genre::name() const
{
    return m_name;
}

std::vector<ArtistPtr> Genre::artists( medialibrary::SortingCriteria, bool desc ) const
{
    std::string req = "SELECT a.* FROM " + policy::ArtistTable::Name + " a "
            "INNER JOIN " + policy::AlbumTrackTable::Name + " att ON att.artist_id = a.id_artist "
            "WHERE att.genre_id = ? GROUP BY att.artist_id"
            " ORDER BY a.name";
    if ( desc == true )
        req += " DESC";
    return Artist::fetchAll<IArtist>( m_ml, req, m_id );
}

std::vector<AlbumTrackPtr> Genre::tracks( medialibrary::SortingCriteria sort, bool desc ) const
{
    return AlbumTrack::fromGenre( m_ml, m_id, sort, desc );
}

std::vector<AlbumPtr> Genre::albums() const
{
    static const std::string req = "SELECT a.* FROM " + policy::AlbumTable::Name + " a "
            "INNER JOIN " + policy::AlbumTrackTable::Name + " att ON att.album_id = a.id_album "
            "WHERE att.genre_id = ? GROUP BY att.album_id";
    return Album::fetchAll<IAlbum>( m_ml, req, m_id );
}

bool Genre::createTable( DBConnection dbConn )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::GenreTable::Name +
        "("
            "id_genre INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT UNIQUE ON CONFLICT FAIL"
        ")";
    static const std::string vtableReq = "CREATE VIRTUAL TABLE IF NOT EXISTS "
                + policy::GenreTable::Name + "Fts USING FTS3("
                "name"
            ")";

    static const std::string vtableInsertTrigger = "CREATE TRIGGER IF NOT EXISTS insert_genre_fts"
            " AFTER INSERT ON " + policy::GenreTable::Name +
            " BEGIN"
            " INSERT INTO " + policy::GenreTable::Name + "Fts(rowid,name) VALUES(new.id_genre, new.name);"
            " END";
    static const std::string vtableDeleteTrigger = "CREATE TRIGGER IF NOT EXISTS delete_genre_fts"
            " BEFORE DELETE ON " + policy::GenreTable::Name +
            " BEGIN"
            " DELETE FROM " + policy::GenreTable::Name + "Fts WHERE rowid = old.id_genre;"
            " END";
    return sqlite::Tools::executeRequest( dbConn, req ) &&
            sqlite::Tools::executeRequest( dbConn, vtableReq ) &&
            sqlite::Tools::executeRequest( dbConn, vtableInsertTrigger ) &&
            sqlite::Tools::executeRequest( dbConn, vtableDeleteTrigger );
}

std::shared_ptr<Genre> Genre::create( MediaLibraryPtr ml, const std::string& name )
{
    static const std::string req = "INSERT INTO " + policy::GenreTable::Name + "(name)"
            "VALUES(?)";
    auto self = std::make_shared<Genre>( ml, name );
    if ( insert( ml, self, req, name ) == false )
        return nullptr;
    return self;
}

std::shared_ptr<Genre> Genre::fromName( MediaLibraryPtr ml, const std::string& name )
{
    static const std::string req = "SELECT * FROM " + policy::GenreTable::Name + " WHERE name = ?";
    return fetch( ml, req, name );
}

std::vector<GenrePtr> Genre::search( MediaLibraryPtr ml, const std::string& name )
{
    static const std::string req = "SELECT * FROM " + policy::GenreTable::Name + " WHERE id_genre IN "
            "(SELECT rowid FROM " + policy::GenreTable::Name + "Fts WHERE name MATCH ?)";
    return fetchAll<IGenre>( ml, req, name + "*" );
}

std::vector<GenrePtr> Genre::listAll( MediaLibraryPtr ml, medialibrary::SortingCriteria, bool desc )
{
    std::string req = "SELECT * FROM " + policy::GenreTable::Name + " ORDER BY name";
    if ( desc == true )
        req += " DESC";
    return fetchAll<IGenre>( ml, req );
}

