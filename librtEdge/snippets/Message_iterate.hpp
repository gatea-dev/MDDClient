
void DumpAllFieldIDs( Message &msg )
{
    for ( msg.reset(); (msg)(); )
      printf( "FID [%04d]\n", msg.field()->Fid() );
}
