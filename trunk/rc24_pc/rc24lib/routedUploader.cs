/*
Copyright 2008 - 2009 © Alan Hopper

	This file is part of rc24.

    rc24 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    rc24 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with rc24.  If not, see <http://www.gnu.org/licenses/>.


*/
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace rc24
{
    public class routedUploader
    {
        int filePos;
        byte[]bin;
        byte crc;
                    
        public routedUploader() {}
        
        public void setFile(string filename)
        {
            bin = File.ReadAllBytes(filename);
            filePos=0;
        }

        public routedMessage sendNextCmd(routedMessage msg)
        {
            if (filePos >= bin.Length)
            {
                byte[]cmd = new byte[2];
                cmd[0] = 0x94;
                cmd[1] = crc;
                return new routedMessage(msg.Route.getReturnRoute(), cmd);
            }
            else
            {
                
                int len = Math.Min(128, bin.Length - filePos);
                byte[] cmd = new byte[len+2];
              
                cmd[0] = 0x92;
                cmd[1] = (byte)len;
                Array.Copy(bin, filePos, cmd, 2, len);
                for (int i = filePos; i < filePos + len; i++)
                {
                    crc ^= bin[i];
                }
                filePos += len;
                
                // Update main form with % complete
                if (null != UploadEvent)
                {
                    int pcentDone = (int)Math.Ceiling(filePos * 100.0 / bin.Length);
                    UploadEvent(this, new UploadEventArgs(pcentDone));
                }
                return new routedMessage(msg.Route.getReturnRoute(), cmd);
            }
        }
        
        // Delegate for the classes events
        public delegate void UploadEventHandler(object sender, UploadEventArgs e);
        
        // Events for uploader
        public event UploadEventHandler UploadEvent;
        
        // Arguments for the uploaded event
        public class UploadEventArgs : EventArgs
        {
            public readonly int percentDone;
            public UploadEventArgs(int newPercentDone)
            {
                percentDone = newPercentDone;
            }
        }
    }
}
