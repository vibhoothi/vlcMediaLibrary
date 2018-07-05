"CREATE TABLE IF NOT EXISTS " + policy::FileTable::Name +
"("
    "id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
    "media_id UNSIGNED INT DEFAULT NULL,"
    "playlist_id UNSIGNED INT DEFAULT NULL,"
    "mrl TEXT,"
    "type UNSIGNED INTEGER,"
    "last_modification_date UNSIGNED INT,"
    "size UNSIGNED INT,"
    "folder_id UNSIGNED INTEGER,"
    "is_present BOOLEAN NOT NULL DEFAULT 1,"
    "is_removable BOOLEAN NOT NULL,"
    "is_external BOOLEAN NOT NULL,"

    "FOREIGN KEY (media_id) REFERENCES " + policy::MediaTable::Name
    + "(id_media) ON DELETE CASCADE,"

    "FOREIGN KEY (playlist_id) REFERENCES " + policy::PlaylistTable::Name
    + "(id_playlist) ON DELETE CASCADE,"

    "FOREIGN KEY (folder_id) REFERENCES " + policy::FolderTable::Name
    + "(id_folder) ON DELETE CASCADE,"

    "UNIQUE( mrl, folder_id ) ON CONFLICT FAIL"
")"