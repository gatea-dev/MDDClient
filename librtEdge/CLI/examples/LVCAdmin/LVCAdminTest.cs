/******************************************************************************
*
*  LVCAdminTest.cs
*     librtEdge .NET LVCAdmin driver
*
*  REVISION HISTORY:
*      7 MAR 2022 jcs  Created.
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Security.Policy;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.Xml.Linq;
using librtEdge;

class LVCAdminTest
{
   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main( String[] args )
   {
       var argc  = args.length;
       var admin = new LVCAdmin(@"localhost:7161");
       var file  = argc ? args[0] : "C:\TEMP\lvc-tickers.xml" );
       var tickerList = GetTickersFromFile( file );

       tickerList = tickerList.OrderBy(x => x).ToArray();
       int i = 0;
       foreach (var t in Partition(tickerList, 5))
       {
           Task.Delay(1000).Wait();
           Console.WriteLine($"Adding tickers={string.Join(",",t.ToArray())}...");
           admin.AddTickers("bloomberg", t.ToArray());
           Task.Delay(1000).Wait();
           i+=5;
       }
 
       Task.Delay(100).Wait(); // time needed to add a subscription...
      
       var lvc = new LVC(@"C:\MDD\MDDClient4win64\gatea\bsys\mdd\db\lvc.bloomberg.dbf");
       var lvcDataAll = lvc.ViewAll();
 
       var schema = lvc.GetSchema();
 
       foreach (var tickerData in lvcDataAll._tkrs)
       {
           if (tickerData._tDead != 0) // skip dead feed
               continue;
 
           Console.WriteLine(
               $"Last Updated: {GetDateTime(tickerData,5)} Ticker: {tickerData._pTkr} Last: {GetDouble(tickerData, 6)}");
       }
 
       Task.Delay(2000).Wait();
       admin.Dispose();
 
   } // Main()
 
   private static string[] GetTickersFromFile(string fileName)
   {
       var doc = new XmlDocument();
       doc.Load(fileName);
 
       var tickers = new List<string>();
       var items = doc.SelectNodes(".//Item");
       if (items != null)
       {
           foreach (XmlNode i in items)
           {
               var ticker = i.Attributes["Name"];
               tickers.Add(ticker.Value);
           }
       }
 
       return tickers.ToArray();
   } // GetTickersFromFile()
 
   private static double? GetDouble(LVCData tickerData, int fieldId)
   {
       var field = tickerData.GetField(fieldId);
 
       if (field == null || field.Type() == rtFldType.rtFld_undef)
           return null;
 
       return InvalidToNull(field.GetAsDouble());
   }
 
   private static DateTime? GetDateTime(LVCData tickerData, int fieldId)
   {
       var field = tickerData.GetField(fieldId);
 
       if (field == null || field.Type() == rtFldType.rtFld_undef)
           return null;
 
       return (DateTime?)(Convert.ToDateTime(field.GetAsDateTime()));
   }
 
   public static bool IsValid(double value)
   {
       return !double.IsInfinity(value) && !double.IsNaN(value);
   }
 
   public static double InvalidToZero(double value)
   {
       return IsValid(value) ? value : 0;
   }
 
   public static double? InvalidToNull(double value)
   {
       return IsValid(value) ? value : (double?)null;
   }
 
   public static IEnumerable<IEnumerable<T>> Partition<T>
       (IEnumerable<T> source, int size)
   {
       T[] array = null;
       int count = 0;
       foreach (T item in source)
       {
           if (array == null)
           {
               array = new T[size];
           }
           array[count] = item;
           count++;
           if (count == size)
           {
               yield return new ReadOnlyCollection<T>(array);
               array = null;
               count = 0;
           }
       }
       if (array != null)
       {
           Array.Resize(ref array, count);
           yield return new ReadOnlyCollection<T>(array);
       }
   } // Partition<T>

} // class LVCAdminTest
