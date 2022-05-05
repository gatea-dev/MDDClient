
void ProcessField( Field &fld )
{
   rtFldType ty;

   ty = fld.Type();
   switch( ty ) {
      case rtFld_int8:
      case rtFld_int16:
      case rtFld_int:
         // Use fld.GetInt32()
         break;
      case rtFld_int64:
         // Use fld.GetInt64()
         break;
      case rtFld_double:
      case rtFld_float:
         // Use fld.GetDouble()
         break;
      case rtFld_string:
      case rtFld_bytestream:
      default:
         // Use fld.GetString()
         break;

   }
}
