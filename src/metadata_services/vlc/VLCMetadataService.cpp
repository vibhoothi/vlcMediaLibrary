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

#include <iostream>

#include "VLCMetadataService.h"
#include "Media.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "Show.h"

#include "Media.h"

VLCMetadataService::VLCMetadataService( const VLC::Instance& vlc )
    : m_instance( vlc )
    , m_cb( nullptr )
    , m_ml( nullptr )
{
}

bool VLCMetadataService::initialize(IMetadataServiceCb* callback, MediaLibrary* ml )
{
    m_cb = callback;
    m_ml = ml;
    return true;
}

unsigned int VLCMetadataService::priority() const
{
    return 100;
}

bool VLCMetadataService::run( std::shared_ptr<Media> file, void* data )
{
    LOG_INFO( "Parsing ", file->mrl() );

    auto ctx = new Context( file );
    std::unique_ptr<Context> ctxPtr( ctx );

    ctx->media = VLC::Media( m_instance, file->mrl(), VLC::Media::FromPath );

    std::unique_lock<std::mutex> lock( m_mutex );
    bool parsed;

    ctx->media.eventManager().onParsedChanged([this, ctx, data, &parsed](bool status) mutable {
        if ( status == false )
            return;
        auto res = handleMediaMeta( ctx->file, ctx->media );
        m_cb->done( ctx->file, res, data );

        std::unique_lock<std::mutex> lock( m_mutex );
        parsed = true;
        m_cond.notify_all();
    });
    ctx->media.parseAsync();
    auto success = m_cond.wait_for( lock, std::chrono::seconds( 5 ), [&parsed]() { return parsed == true; } );
    if ( success == false )
        m_cb->done( ctx->file, StatusFatal, data );
}

ServiceStatus VLCMetadataService::handleMediaMeta( std::shared_ptr<Media> file, VLC::Media& media ) const
{
    auto tracks = media.tracks();
    if ( tracks.size() == 0 )
    {
        LOG_ERROR( "Failed to fetch tracks" );
        return StatusFatal;
    }
    bool isAudio = true;
    for ( auto& track : tracks )
    {
        auto codec = track.codec();
        std::string fcc( (const char*)&codec, 4 );
        if ( track.type() == VLC::MediaTrack::Video )
        {
            isAudio = false;
            auto fps = (float)track.fpsNum() / (float)track.fpsDen();
            file->addVideoTrack( fcc, track.width(), track.height(), fps );
        }
        else if ( track.type() == VLC::MediaTrack::Audio )
        {
            file->addAudioTrack( fcc, track.bitrate(), track.rate(),
                                      track.channels() );
        }
    }
    auto duration = media.duration();
    file->setDuration( duration );
    if ( isAudio == true )
    {
        if ( parseAudioFile( file, media ) == false )
            return StatusFatal;
    }
    else
    {
        if (parseVideoFile( file, media ) == false )
            return StatusFatal;
    }
    return StatusSuccess;
}

bool VLCMetadataService::parseAudioFile( std::shared_ptr<Media> media, VLC::Media& vlcMedia ) const
{
    media->setType( IMedia::Type::AudioType );
    auto albumTitle = vlcMedia.meta( libvlc_meta_Album );
    auto newAlbum = false;
    std::shared_ptr<Album> album;
    if ( albumTitle.length() > 0 )
    {
        album = std::static_pointer_cast<Album>( m_ml->album( albumTitle ) );
        if ( album == nullptr )
        {
            album = m_ml->createAlbum( albumTitle );
            if ( album != nullptr )
            {
                newAlbum = true;
                auto date = vlcMedia.meta( libvlc_meta_Date );
                if ( date.length() > 0 )
                    album->setReleaseDate( std::stoul( date ) );
                auto artwork = vlcMedia.meta( libvlc_meta_ArtworkURL );
                if ( artwork.length() != 0 )
                    album->setArtworkUrl( artwork );
            }
        }
    }
    std::shared_ptr<AlbumTrack> track;
    if ( album != nullptr )
    {
        track = handleTrack( album, media, vlcMedia );
        if ( track != nullptr )
            media->setAlbumTrack( track );
    }

    return handleArtist( album, media, vlcMedia, newAlbum );
}

bool VLCMetadataService::parseVideoFile( std::shared_ptr<Media> file, VLC::Media& media ) const
{
    file->setType( IMedia::Type::VideoType );
    auto title = media.meta( libvlc_meta_Title );
    if ( title.length() == 0 )
        return true;
    auto showName = media.meta( libvlc_meta_ShowName );
    if ( showName.length() == 0 )
    {
        auto show = m_ml->show( showName );
        if ( show == nullptr )
        {
            show = m_ml->createShow( showName );
            if ( show == nullptr )
                return false;
        }

        auto episodeIdStr = media.meta( libvlc_meta_Episode );
        if ( episodeIdStr.length() > 0 )
        {
            size_t endpos;
            int episodeId = std::stoi( episodeIdStr, &endpos );
            if ( endpos != episodeIdStr.length() )
            {
                LOG_ERROR( "Invalid episode id provided" );
                return true;
            }
            std::shared_ptr<Show> s = std::static_pointer_cast<Show>( show );
            s->addEpisode( title, episodeId );
        }
    }
    else
    {
        // How do we know if it's a movie or a random video?
    }
    return true;
}

bool VLCMetadataService::handleArtist( std::shared_ptr<Album> album, std::shared_ptr<Media> media, VLC::Media& vlcMedia, bool newAlbum ) const
{
    assert(media != nullptr);

    auto newArtist = false;
    auto artistName = vlcMedia.meta( libvlc_meta_Artist );
    if ( artistName.length() != 0 )
    {
        auto artist = std::static_pointer_cast<Artist>( m_ml->artist( artistName ) );
        if ( artist == nullptr )
        {
            artist = m_ml->createArtist( artistName );
            if ( artist == nullptr )
            {
                LOG_ERROR( "Failed to create new artist ", artistName );
                // Consider this a minor failure and go on nevertheless
                return true;
            }
            newArtist = true;
        }
        media->addArtist( artist );
        // If this is either a new album or a new artist, we need to add the relationship between the two.
        if ( album != nullptr && ( newAlbum == true || newArtist == true ) )
        {
            if ( album->addArtist( artist ) == false )
                LOG_WARN( "Failed to add artist ", artist->name(), " to album ", album->title() );
        }
    }
    else
        media->addArtist( nullptr );
    return true;
}

std::shared_ptr<AlbumTrack> VLCMetadataService::handleTrack(std::shared_ptr<Album> album, std::shared_ptr<Media> media, VLC::Media& vlcMedia) const
{
    auto trackNbStr = vlcMedia.meta( libvlc_meta_TrackNumber );
    if ( trackNbStr.length() == 0 )
    {
        LOG_WARN( "Failed to get track id" );
        return nullptr;
    }

    auto title = vlcMedia.meta( libvlc_meta_Title );
    if ( title.length() == 0 )
    {
        LOG_WARN( "Failed to get track title" );
        title = "Track #";
        title += trackNbStr;
    }
    media->setTitle( title );
    unsigned int trackNb = std::stoi( trackNbStr );
    auto track = std::static_pointer_cast<AlbumTrack>( album->addTrack( media, trackNb ) );
    if ( track == nullptr )
    {
        LOG_ERROR( "Failed to create album track" );
        return nullptr;
    }
    auto genre = vlcMedia.meta( libvlc_meta_Genre );
    if ( genre.length() != 0 )
    {
        track->setGenre( genre );
    }
    return track;
}
