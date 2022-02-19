using System;
using System.Collections.Generic;
using System.Text;

namespace KinoMetGUI
{
    public enum GBAMODE
    {
        MODE_3,
        MODE_5,
    }

    /// <summary>
    /// These settings are only in reference to encoding options to be represented on the GUI.
    /// </summary>
    public class KiNoMetSettings
    {
        public Dictionary<GBAMODE, string> screenChoices = new Dictionary<GBAMODE, string>()
            {
                {GBAMODE.MODE_3, "240x160" },{GBAMODE.MODE_5,"192x160" }
            };
        public class Defaults
        {
            GBAMODE defaultMode = GBAMODE.MODE_3;
            int defaultQuality = 31;

        }
        /// <summary>
        /// Determines how big the picture will be, and how to render on the gba.
        /// Decoder will know by screen size.
        /// </summary>
        public GBAMODE mode;
        public int quality;
        KiNoMetSettings()
        {
            //Set defaults
        }
    }
}
