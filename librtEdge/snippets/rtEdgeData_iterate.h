
void DumpAllFieldIDs( rtEdgeData &msg )
{
    for ( msg.reset(); (msg)(); printf( "FID [%04d]\n", msg.field()->Fid() );
}
