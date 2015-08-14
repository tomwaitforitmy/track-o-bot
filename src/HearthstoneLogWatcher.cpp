#include "HearthstoneLogWatcher.h"
#include "Hearthstone.h"

#include <QFile>
#include <QTimer>

HearthstoneLogWatcher::HearthstoneLogWatcher()
  : mPath( Hearthstone::Instance()->LogPath() ), mLastSeekPos( 0 )
{
  // We used QFileSystemWatcher before but it fails on windows
  // Windows File Notification seems to be very tricky with files
  // which are not explicitly flushed (which is the case for the Hearthstone Log)
  // QFileSystemWatcher fails, manual implemention with FindFirstChangeNotification
  // fails. So instead of putting too much work into a file-system depending solution
  // just use a low-overhead polling strategy
  QTimer *timer = new QTimer( this );
  connect( timer, SIGNAL( timeout() ), this, SLOT( CheckForLogChanges() ) );
  timer->start( CHECK_FOR_LOG_CHANGES_INTERVAL_MS );

  QFile file( mPath );
  if( file.exists() ) {
    mLastSeekPos = file.size();
  }
}

void HearthstoneLogWatcher::CheckForLogChanges() {
  // Only access disk when HS is running
  if( !Hearthstone::Instance()->Running() ) {
    return;
  }

  QFile file( mPath );
  if( !file.open( QIODevice::ReadOnly ) ) {
    return;
  }

  qint64 size = file.size();
  if( size < mLastSeekPos ) {
    LOG( "Last seek pos is smaller than current size. Truncation!" );
    mLastSeekPos = size;
  } else {
    // Use raw QFile instead of QTextStream
    // QTextStream uses buffering and seems to skip some lines (see also QTextStream#pos)
    file.seek( mLastSeekPos );

    QByteArray buf = file.readAll();
    QList< QByteArray > lines = buf.split('\n');

    QByteArray lastLine = lines.takeLast();
    for( const QByteArray& line : lines ) {
      emit LineAdded( QString::fromUtf8( line.trimmed() ) );
    }

    mLastSeekPos = file.pos() - lastLine.size();
  }
}

