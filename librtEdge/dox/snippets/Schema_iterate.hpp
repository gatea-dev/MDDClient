
void DumpAllFieldIDs( Schema &sch )
{
    for ( sch.reset(); sch.forth(); )
      printf( "FID [%04d]\n", sch.field()->Fid() );
}
