﻿using System;
using System.Collections.Generic;
using System.IO;

namespace KinometGui
{
    public class ROM
    {
        private static List<string> headerLines = new List<string>();
        private static List<string> sourceLines = new List<string>();
        //Line counter 
        private static string comment = "//---------------------------------------------------------------------------------";
        public static void MakeSource(string ArrayName, byte[] buffer, string outputdir)
        {
            //---------------------------------------------------------------------------------
            UInt64 counter = 0UL;
            UInt64 length = (ulong)buffer.Length;

            ROM.headerLines.Add($"#ifndef _{ArrayName}_h_\n");
            ROM.headerLines.Add($"#define _{ArrayName}_h_\n");
            ROM.headerLines.Add(comment);
            ROM.headerLines.Add($"extern const unsigned char {ArrayName}[];\n");
            ROM.headerLines.Add($"extern const int {ArrayName}_size;\n");
            ROM.headerLines.Add(comment);
            ROM.headerLines.Add($"#endif //_{ArrayName}_h_\n");
            ROM.headerLines.Add(comment);


            //ROM.sourceLines.Add($"char* ArrayName_{ArrayName} = \"{ArrayName}\";\n");//so we know what file we're actually fucking with.

            ROM.sourceLines.Add($"const unsigned char {ArrayName}[] = {{\n\t");
            string thislIne = "";
            while (counter < length)
            {
                thislIne += "0x" + buffer[counter++].ToString("X2") + ",";

                if ((int)(counter % 16) == 0)
                {
                    ROM.sourceLines.Add(thislIne);
                    thislIne = "";
                }
            }

            if (!string.IsNullOrEmpty(thislIne))
            {
                ROM.sourceLines.Add(thislIne);
            }

            ROM.sourceLines.Add($"\n}};\n");
            ROM.sourceLines.Add($"const int {ArrayName}_size = sizeof({ArrayName});\n");
            return;
        }


        public static void Write(string outputdir, string file)
        {
            File.WriteAllLines($"{outputdir}\\{file}.h", ROM.headerLines);
            List<string> newRom = new List<string> (){ $"#include \"{file}.h\"" };
            newRom.InsertRange(1, sourceLines);
            File.WriteAllLines($"{outputdir}\\{file}.cpp", newRom);
            headerLines = new List<string>();
            sourceLines = new List<string>();
        }

        //lol
        public static void MakeSource(string ArrayName, sbyte[] buffer, string outputdir)
        {
            //---------------------------------------------------------------------------------
            UInt64 counter = 0UL;
            UInt64 length = (ulong)buffer.Length;

            ROM.headerLines.Add($"#ifndef _{ArrayName}_h_\n");
            ROM.headerLines.Add($"#define _{ArrayName}_h_\n");
            ROM.headerLines.Add(comment);
            ROM.headerLines.Add($"extern const unsigned char {ArrayName}[];\n");
            ROM.headerLines.Add($"extern const int {ArrayName}_size;\n");
            ROM.headerLines.Add(comment);
            ROM.headerLines.Add($"#endif //_{ArrayName}_h_\n");
            ROM.headerLines.Add(comment);

            ROM.sourceLines.Add($"char* ArrayName_{ArrayName} = \"{ArrayName}\";\n");//so we know what file we're actually fucking with.
            ROM.sourceLines.Add($"const unsigned char {ArrayName}[] = {{\n\t");
            string thislIne = "";
            while (counter < length)
            {
                thislIne += "0x" + buffer[counter++].ToString("X2") + ",";

                if ((int)(counter % 16) == 0)
                {
                    ROM.sourceLines.Add(thislIne);
                    thislIne = "";
                }
            }

            if (!string.IsNullOrEmpty(thislIne))
            {
                ROM.sourceLines.Add(thislIne);
            }

            ROM.sourceLines.Add($"\n}};\n");
            ROM.sourceLines.Add($"const int {ArrayName}_size = sizeof({ArrayName});\n");


            return;
        }


    }
}
