TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS = psflib \
          QSoundCore/Core \
          app

app.depends = psflib \
              QSoundCore/Core
