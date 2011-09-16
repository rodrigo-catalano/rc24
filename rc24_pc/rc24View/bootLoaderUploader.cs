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
using System.Windows.Forms;
using System.IO.Ports;
using System.IO;
using System.Threading;

namespace Serial
{
    public class bootLoaderUploader
    {
        private uint start;
        private uint lenremaining = 0;
        private uint packetsize = 128;
        private byte[] bin;
        private SerialPort inp;
        private byte sector = 0;
        TextBox tbstatus;
        TextBox tbm;

        private uint textstart;
        private uint textlen;
        private uint datastart;
        private uint datalen;
        private uint flashSectorSize = 0x00008000;

        System.IO.FileStream dumpfile;

        private string file5139;
        private string file5148;



        public bootLoaderUploader()
        {

        }
        public void clearOneWireBus()
        {
            //hold bus low to break bootloader naking it's own naks 

            inp.RtsEnable = true;

            Thread.Sleep(16000);
            inp.RtsEnable = false;

        }
        public void timeout()
        {
            //
            clearOneWireBus();
            // resend last packet


        }
        public void sendPacketEx(byte[] buff, int start, byte length, byte cmd)
        {

            JennicPacket.Write(inp, buff, start, length, cmd);
        }
        public void upload(string filename5139,string filename5148, SerialPort sp, TextBox status, TextBox dbg)
        {
            //quick and dirty fix for JN5148
            file5139 = filename5139;
            file5148 = filename5148;
            
            tbstatus = status;
            tbm = dbg;
            inp = sp;
            bin = File.ReadAllBytes(filename5139);
            lenremaining = (uint)bin.Length;
            start = 0;
            JennicPacket.Write(inp, null, 0, 0, 0x25);

            tbstatus.Text = "Connecting to module bootloader";

        }
        private void LoadJN5148File()
        {
            bin = File.ReadAllBytes(file5148);
            lenremaining = (uint)bin.Length;
            start = 0;
            
        }
        public void dump(SerialPort sp, TextBox status, TextBox dbg)
        {
            tbstatus = status;
            tbm = dbg;
            inp = sp;

            System.IO.FileStream dumpfile = new FileStream("jendump.bin", FileMode.Create);

            start = 0;
            byte[] buf = new byte[6];
            buf[0] = (byte)((start) & 0x000000ff);
            buf[1] = (byte)((start >> 8 & 0x000000ff));
            buf[2] = (byte)((start >> 16) & 0x000000ff);
            buf[3] = (byte)(start >> 24);

            buf[4] = 128;
            buf[5] = 0;

            JennicPacket.Write(inp, buf, 0, 6, 0x1f);
        }
        public void runFromMem(string filename, SerialPort sp, TextBox status, TextBox dbg)
        {
            tbstatus = status;
            tbm = dbg;
            inp = sp;
            bin = File.ReadAllBytes(filename);

            textstart = (uint)((bin[0x04] << 24) + (bin[0x05] << 16) + (bin[0x06] << 8) + (bin[0x07]));
            textlen = (uint)((bin[0x08] << 24) + (bin[0x09] << 16) + (bin[0x0a] << 8) + (bin[0x0b]));
            datastart = (uint)((bin[0x0c] << 24) + (bin[0x0d] << 16) + (bin[0x0e] << 8) + (bin[0x0f]));
            datalen = (uint)((bin[0x10] << 24) + (bin[0x11] << 16) + (bin[0x12] << 8) + (bin[0x13]));

            textstart = 0x04000FD0;

            lenremaining = (uint)bin.Length;// -36;
            start = textstart;// 0xf0000000;
            tbstatus.Text = "Connecting to tx";




            writeNextBlockMem();

        }
        public void writeNextBlockMem()
        {
            tbstatus.Text = "Downloading " + ((start - textstart) * 100 / bin.Length) + "%";

            uint plen = Math.Min(packetsize, lenremaining);
            byte[] buf = new byte[256];
            buf[0] = (byte)((start) & 0x000000ff);
            buf[1] = (byte)((start >> 8 & 0x000000ff));
            buf[2] = (byte)((start >> 16) & 0x000000ff);
            buf[3] = (byte)(start >> 24);

            Array.Copy(bin, start - textstart, buf, 4, plen);

            Thread.Sleep(1000);
            JennicPacket.Write(inp, buf, 0, (byte)(plen + 4), 0x1d);



            lenremaining -= plen;
            start += plen;

        }

        public void writeNextBlock()
        {
            tbstatus.Text = "Downloading " + (start * 100 / bin.Length) + "%";

            uint plen = Math.Min(packetsize, lenremaining);
            byte[] buf = new byte[256];
            buf[0] = (byte)((start) & 0x000000ff);
            buf[1] = (byte)((start >> 8 & 0x000000ff));
            buf[2] = (byte)((start >> 16) & 0x000000ff);
            buf[3] = (byte)(start >> 24);

            Array.Copy(bin, start, buf, 4, plen);

            //        Thread.Sleep(16000);
            JennicPacket.Write(inp, buf, 0, (byte)(plen + 4), 0x09);



            lenremaining -= plen;
            start += plen;

        }

        public bool finished()
        {
            return lenremaining == 0;
        }

        public bool processJennicMsg(byte[] buff)
        {
            //if
            //ignore echo of own packets
            //after ack module will be sending out errors
            //
            if (buff[0] == 0) return true;
            //    if((buff[1]&0x01)==0)clearOneWireBus();

            //       tbm.AppendText("-"+buff[0]+" " + buff[1].ToString("X2"));
            byte len = buff[0];
            byte checksum = 0;
            for (int i = 0; i < len; i++)
            {
                byte b = buff[i];
                checksum ^= b;
            }
            if (buff[len] != checksum)
            {
                return false;
            }
            byte cmd = buff[1];

            switch (cmd)
            {
                case 0x26: //Read Flash ID response
                    {
                        if (buff[2] == 0)
                        {
                            byte flashType = 255;

                            if (buff[3] == 0x10 && buff[4] == 0x10) flashType = 0;
                            if (buff[3] == 0xbf && buff[4] == 0x49) flashType = 1;
                            if (buff[3] == 0x1f && buff[4] == 0x60) flashType = 2;
                            if (buff[3] == 0x12 && buff[4] == 0x12)
                            {
                                flashType = 3;
                                flashSectorSize = 0x00010000;
                                LoadJN5148File();
                            }
                            if (flashType != 255)
                            {
                                byte[] buf = new byte[5];
                                buf[0] = flashType;
                                buf[1] = 0;
                                buf[2] = 0;
                                buf[3] = 0;
                                buf[4] = 0;
                                tbstatus.Text = "Got Flash ID";

                                //      Thread.Sleep(200);

                                JennicPacket.Write(inp, buf, 0, 5, 0x2c);
                            }
                        }
                        break;
                    }
                case 0x2d: //Set Flash ID response
                    {
                        if (buff[2] == 0)
                        {
                            //read mac
                            byte[] buf = new byte[6];
                            buf[0] = 48;
                            buf[1] = 0;
                            buf[2] = 0;
                            buf[3] = 0;
                            buf[4] = 32;
                            buf[5] = 0;

                            tbstatus.Text = "Set Fash ID";
                            //  Thread.Sleep(200);

                            JennicPacket.Write(inp, buf, 0, 6, 0x0b);
                        }
                        break;
                    }
                case 0x0c: //Read Flash response
                    {
                        if (buff[2] == 0)
                        {
                            //patch in mac address and zigbee stuff
                            Array.Copy(buff, 3, bin, 48, 32);
                            //check valid mac address
                            int macsum = 0;
                            for (int i = 0; i < 8; i++)
                            {
                                macsum += buff[i + 3];
                            }
                            if (macsum == 0 || macsum == 8 * 0xff)
                            {
                                getmacdialog gmd = new getmacdialog();
                                if (gmd.ShowDialog() != DialogResult.OK)
                                {
                                    break;
                                }
                                else
                                {
                                    Array.Copy(gmd.mac, 0, bin, 48, 8);
                                }
                            }

                            //set status register  
                            byte[] buf = new byte[1];
                            buf[0] = 0;
                            //      Thread.Sleep(16000);

                            JennicPacket.Write(inp, buf, 0, 1, 0x0f);
                            sector = 0;
                            tbstatus.Text = "Got Mac";


                        }
                        break;
                    }
                case 0x10: //set status response
                    {
                        if (buff[2] == 0)
                        {
                            //clear flash
                            byte[] buf = new byte[1];
                            buf[0] = sector;
                            //    Thread.Sleep(200);

                            JennicPacket.Write(inp, buf, 0, 1, 0x0d);
                       //     JennicPacket.Write(inp, buf, 0, 0, 0x07);
                            tbstatus.Text = "Set Status";


                        }
                        break;
                    }
                case 0x08: //clear all response
                    {
                        if (buff[2] == 0)
                        {
                            writeNextBlock();
                        }
                        else
                        {
                            MessageBox.Show("clear all fail. code - " + buff[2]);
                        }
                        break;
                    }
                case 0x0e: //clear sector response
                    {
                        if (buff[2] == 0)
                        {
                            sector++;
                            if (sector < 2)
                            {
                                //clear flash
                                byte[] buf = new byte[1];
                                buf[0] = sector;
                                Thread.Sleep(200);
                                JennicPacket.Write(inp, buf, 0, 1, 0x0d);

                            }
                            else
                            {
                                writeNextBlock();
                            }
                            tbstatus.Text = "Cleared Sector " + (sector - 1);

                        }
                        break;
                    }
                case 0x0a: //write flash
                    {
                        if (buff[2] == 0)
                        {
                            if (lenremaining > 0)
                            {
                                writeNextBlock();

                            }
                            else
                            {
                                // try reset - won't work in bootloader mode
                                JennicPacket.Write(inp, null, 0, 0, 0x80);
                                tbstatus.Text = "Done";

                                return true;
                            }
                        
                       
                        }
                        else
                        {
                            MessageBox.Show("Write Failure. code - " + buff[2]);
                        }
                        break;
                    }
                case 0x1E: //write ram
                    {
                        if (buff[2] == 0)
                        {
                            if (lenremaining > 0)
                            {
                                writeNextBlockMem();

                            }
                            else
                            {
                                // run code
                                byte[] buf = new byte[4];
                                buf[0] = bin[0x23];
                                buf[1] = bin[0x22];
                                buf[2] = bin[0x21];
                                buf[3] = bin[0x20];

                                Thread.Sleep(1500);
                                JennicPacket.Write(inp, buf, 0, 4, 0x21);
                                tbstatus.Text = "Done";

                                return true;
                            }
                        }
                        else
                        {

                        }
                        break;
                    }
                case 0x22: //run
                    {
                        tbstatus.Text = "Running";
                        Thread.Sleep(1500);

                        break;
                    }
                case 0x20:
                    {
                        start += 128;
                        tbstatus.Text = "Dumping " + start;

                        //        dumpfile.Write(buff, 3, 128);
                        if (start < 0x20000)
                        {
                            byte[] buf = new byte[6];
                            buf[0] = (byte)((start) & 0x000000ff);
                            buf[1] = (byte)((start >> 8 & 0x000000ff));
                            buf[2] = (byte)((start >> 16) & 0x000000ff);
                            buf[3] = (byte)(start >> 24);

                            buf[4] = 128;
                            buf[5] = 0;

                            JennicPacket.Write(inp, buf, 0, 6, 0x1f);
                        }
                        else
                        {
                            dumpfile.Close();
                        }
                        break;

                    }
                
            }
            return false;
        }
    }

}
