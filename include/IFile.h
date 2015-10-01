#ifndef IFILE_H
#define IFILE_H

#include <vector>
#include <memory>

#include "IMediaLibrary.h"

class IAlbumTrack;
class IShowEpisode;
class ITrackInformation;

class IFile
{
    public:
        enum class Type : uint8_t
        {
            VideoType, // Any video file, not being a tv show episode
            AudioType, // Any kind of audio file, not being an album track
            UnknownType
        };
        virtual ~IFile() {}

        virtual unsigned int id() const = 0;
        virtual Type type() = 0;
        virtual bool setType( Type type ) = 0;
        virtual AlbumTrackPtr albumTrack() = 0;
        virtual bool setAlbumTrack(AlbumTrackPtr albumTrack ) = 0;
        virtual unsigned int duration() const = 0;
        virtual bool setDuration( unsigned int duration) = 0;
        virtual std::shared_ptr<IShowEpisode> showEpisode() = 0;
        virtual bool setShowEpisode( ShowEpisodePtr showEpisode ) = 0;
        virtual int playCount() const = 0;
        virtual const std::string& mrl() const = 0;
        virtual bool addLabel( LabelPtr label ) = 0;
        virtual bool removeLabel( LabelPtr label ) = 0;
        virtual MoviePtr movie() = 0;
        virtual bool setMovie( MoviePtr movie ) = 0;
        virtual std::vector<std::shared_ptr<ILabel> > labels() = 0;
        virtual bool addVideoTrack( const std::string& codec, unsigned int width,
                                    unsigned int height, float fps ) = 0;
        virtual std::vector<VideoTrackPtr> videoTracks() = 0;
        virtual bool addAudioTrack( const std::string& codec, unsigned int bitrate,
                                    unsigned int sampleRate, unsigned int nbChannels ) = 0;
        virtual std::vector<AudioTrackPtr> audioTracks() = 0;
        // Returns the location of this file snapshot.
        // This is likely to be used for album arts as well.
        virtual const std::string& snapshot() = 0;
        virtual bool setSnapshot( const std::string& snapshot ) = 0;
        /// Returns wether the file has been added as a stand alone file (true), or as
        /// part of a folder (false)
        virtual bool isStandAlone() = 0;
        /// Explicitely mark a file as fully parsed, meaning no metadata service needs to run anymore.
        //FIXME: This lacks granularity as we don't have a straight forward way to know which service
        //needs to run or not.
        virtual bool markParsed() = 0;
        virtual bool isParsed() const = 0;

        virtual unsigned int lastModificationDate() = 0;
};

#endif // IFILE_H
