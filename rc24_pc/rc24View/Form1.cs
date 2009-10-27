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
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO.Ports;
using System.IO;
using System.Threading;
using rc24;

/* This communicates with jennic modules through a logic level serial port.
 * The communications protocol is based on that used by jennic for their
 * flash programmer. See JN-AN-1007-Boot-Loader-Serial-Protocol-1v7.pdf
 * 
 * This program can be used inplace of the jennic programmer for programming
 * It will program both in bootload mode or with the module tx app code
 * running
 * 
 * The rx can be reprogrammed over the radio link from a tx connected to the 
 * serial port once the rx has the rx code initially loaded via the bootloader.
 * 
 */


namespace Serial
{
    public partial class Form1 : Form
    {
        private SerialPort inp = new SerialPort("COM7", 38400, Parity.None, 8, StopBits.One);

        bootLoaderUploader blupload = new bootLoaderUploader();
        rc24.routedUploader rUpload;

        //      private BackgroundWorker backgroundWorker1;

        delegate void SetTextCallback(string text);
        delegate void jennicMessageCallback(byte[] buff);


        byte[] jennicMsgbuff = new byte[256];
        byte jennicMsgLen = 0;
        byte jennicMsgNextIdx = 0;

        const byte SERIALCON = 0;

        DateTime lastData = DateTime.Now;

        routedTreeWalker treeWalker;

        routedNode pcNode = new routedNode("PC");
        routedNode activeNode;


        //      Flobbster.Windows.Forms.PropertyTable nodeParameterList = new Flobbster.Windows.Forms.PropertyTable();
        Flobbster.Windows.Forms.PropertyBag nodeParameterList = new Flobbster.Windows.Forms.PropertyBag();


        public Form1()
        {
            InitializeComponent();

            string com = System.Configuration.ConfigurationManager.AppSettings.GetValues("comport")[0];

            inp.DataReceived += new SerialDataReceivedEventHandler(inp_DataReceived);
            try
            {
                inp.PortName = com;
                inp.Open();
                comboBoxPort.Text = com;
                inp.RtsEnable = false;

            }
            catch (Exception)
            {
            }
            if (inp.IsOpen) buttonConnect.Text = "Disconnect";
            else buttonConnect.Text = "Connect";

            lcd1.MouseClick += new MouseEventHandler(lcd1_MouseClick);

            nodeParameterList.GetValue += new Flobbster.Windows.Forms.PropertySpecEventHandler(nodeParameterList_GetValue);

            nodeParameterList.SetValue += new Flobbster.Windows.Forms.PropertySpecEventHandler(nodeParameterList_SetValue);
            propertyGrid1.SelectedObject = nodeParameterList;

        }

        void nodeParameterList_SetValue(object sender, Flobbster.Windows.Forms.PropertySpecEventArgs e)
        {
            string paramName = e.Property.Name;

            ccParameter param = activeNode.properties[paramName];
            param.Value = e.Value;
            byte[] cmd=param.buildSetCmd();

            if (cmd != null)
            {
                routedMessage msgValSet = new routedMessage(activeNode.address, cmd);
                SendRoutedMessage(msgValSet, SERIALCON);
            }


        }

        void nodeParameterList_GetValue(object sender, Flobbster.Windows.Forms.PropertySpecEventArgs e)
        {
            //e.Property.Name
            e.Value = activeNode.properties[e.Property.Name].Value;

        }

        void lcd1_MouseClick(object sender, MouseEventArgs e)
        {
            byte[] cmd = new byte[3];
            cmd[0] = 0xa0;
            cmd[1] = (byte)(e.X / 2);
            cmd[2] = (byte)(e.Y / 2);
            routedMessage msg = new routedMessage(route.DirectLink, cmd);
            SendRoutedMessage(msg, SERIALCON);
        }


        void inp_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            int i = inp.BytesToRead;
            // packet timeout 
            DateTime t = DateTime.Now;
            if (t.Subtract(lastData).TotalMilliseconds > 200)
            {
                jennicMsgNextIdx = 0;
            }
            lastData = t;

            for (int n = 0; n < i; n++)
            {
                int c = inp.ReadByte();

                if (jennicMsgNextIdx == 0)
                {
                    jennicMsgLen = (byte)c;
                }
                jennicMsgbuff[jennicMsgNextIdx++] = (byte)c;


                if (jennicMsgNextIdx == jennicMsgLen + 1)
                {
                    if (jennicMsgLen > 0)
                    {
                        //if response clear buff and lock line

                        /*     if ((((jennicMsgbuff[1] & 0x01) == 0 || jennicMsgbuff[1] == 0x2d)) && jennicMsgbuff[1] != 0x2c && (jennicMsgbuff[2] == 0))
                             {
                                 inp.RtsEnable = true;
                                 inp.ReadExisting();
                             }
                             */
                        if (((jennicMsgbuff[1] == 0x1e || jennicMsgbuff[1] == 0x22)) && (jennicMsgbuff[2] == 0))
                        {
                            inp.RtsEnable = true;
                            inp.ReadExisting();
                        }

                        processJennicMsg(jennicMsgbuff);
                    }
                    jennicMsgNextIdx = 0;
                }

            }

        }

        private void buttonConnect_Click(object sender, EventArgs e)
        {
            if (inp.IsOpen)
            {
                inp.Close();
            }
            else
            {
                try
                {
                    inp.PortName = comboBoxPort.Text;
                    inp.Open();
                }
                catch (Exception)
                {

                }
            }
            if (inp.IsOpen) buttonConnect.Text = "Disconnect";
            else buttonConnect.Text = "Connect";

        }
        private void SetText(string text)
        {
            // InvokeRequired required compares the thread ID of the
            // calling thread to the thread ID of the creating thread.
            // If these threads are different, it returns true.
            if (this.textBox1.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(SetText);
                this.Invoke(d, new object[] { text });
            }
            else
            {
                textBox1.AppendText(text);
            }
        }
        private void SetStatusText(string text)
        {
            if (this.textBox1.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(SetStatusText);
                this.Invoke(d, new object[] { text });
            }
            else
            {
                textBoxStatus.Text = text;
            }
        }
        private void buttonProgTX_Click(object sender, EventArgs e)
        {
            string filename = System.Configuration.ConfigurationManager.AppSettings.GetValues("txbinpath")[0];
            string filename5148 = System.Configuration.ConfigurationManager.AppSettings.GetValues("txbinpath5148")[0];
            blupload.upload(filename, filename5148, inp, textBoxStatus, textBox1);


            //    blupload.runFromMem("C:\\Jennic\\cygwin\\jennic\\SDK\\Application\\onewirebootloader\\JN5139_Build\\Release\\onewirebootloader.bin", inp, textBoxStatus, textBox1);
        }

        private void buttonProgRx_Click(object sender, EventArgs e)
        {
            string filename = System.Configuration.ConfigurationManager.AppSettings.GetValues("rxbinpath")[0];
            string filename5148 = System.Configuration.ConfigurationManager.AppSettings.GetValues("rxbinpath5148")[0];
            blupload.upload(filename, filename5148, inp, textBoxStatus, textBox1);


            //blupload.upload("C:\\Jennic\\cygwin\\jennic\\SDK\\Application\\onewirebootloader\\JN5139_Build\\Release\\onewirebootloader.bin", inp, textBoxStatus, textBox1);

        }

        private void processJennicMsg(byte[] buff)
        {
            if (this.InvokeRequired)
            {
                jennicMessageCallback d = new jennicMessageCallback(processJennicMsg);
                this.Invoke(d, new object[] { buff });
            }
            else
            {
                byte len = buff[0];
                byte checksum = 0;
                for (int i = 0; i < len; i++)
                {
                    byte b = buff[i];
                    checksum ^= b;
                }
                if (buff[len] != checksum)
                {
                    return;
                }
                byte cmd = buff[1];

                if (cmd == 0x90)
                {
                    //debug message
                    StringBuilder sb = new StringBuilder();
                    for (int i = 2; i < len; i++) sb.Append((char)buff[i]);

                    SetText(sb.ToString());
                }
                else if (cmd == 0x92)
                {
                    lcd1.UpdateBlock((int)buff[2], buff, 3);
                }
                else
                {
                    // StringBuilder sb = new StringBuilder();
                    // for (int i = 0; i <= len; i++) sb.Append((char)buff[i]);
                    //  SetStatusText(sb.ToString());
                }
                if (cmd < 0x30)
                {
                    blupload.processJennicMsg(buff);
                }
                if (cmd == 0xff)
                {
                    pcHandleRoutedMessage(buff, 2, len - 2, 0);
                }
                else
                {
                }

                return;
            }
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            string[] ports = SerialPort.GetPortNames();
            foreach (string port in ports)
            {
                comboBoxPort.Items.Add(port);
            }
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            timer1.Enabled = false;
            blupload.timeout();
        }

        private void buttonimageimport_Click(object sender, EventArgs e)
        {
            OpenFileDialog fd = new OpenFileDialog();
            StringBuilder code = new StringBuilder();

            if (fd.ShowDialog(this) == DialogResult.OK)
            {
                code.Append("uint8 " + Path.GetFileNameWithoutExtension(fd.FileName) + "[] = {");
                bool first = true;
                Bitmap img = new Bitmap(fd.FileName);
                for (int y = 0; y < img.Height; y += 8)
                {
                    for (int x = 0; x < img.Width; x++)
                    {
                        byte b = 0;
                        if (img.GetPixel(x, y).GetBrightness() < 0.5) b += 1;
                        if (img.GetPixel(x, y + 1).GetBrightness() < 0.5) b += 2;
                        if (img.GetPixel(x, y + 2).GetBrightness() < 0.5) b += 4;
                        if (img.GetPixel(x, y + 3).GetBrightness() < 0.5) b += 8;
                        if (img.GetPixel(x, y + 4).GetBrightness() < 0.5) b += 16;
                        if (img.GetPixel(x, y + 5).GetBrightness() < 0.5) b += 32;
                        if (img.GetPixel(x, y + 6).GetBrightness() < 0.5) b += 64;
                        if (img.GetPixel(x, y + 7).GetBrightness() < 0.5) b += 128;
                        if (first)
                        {
                            first = false;
                            code.Append("" + b);
                        }
                        else
                        {
                            code.Append("," + b);
                        }
                    }

                    code.Append("\r\n\t");

                }
                code.Append("};");
                img.Dispose();

            }
            Clipboard.SetText(code.ToString());
        }

        private void buttonRefreshTree_Click(object sender, EventArgs e)
        {
            pcNode.children.Clear();
            bindtree(pcNode);
            treeWalker = new routedTreeWalker(pcNode);
            SendRoutedMessage(treeWalker.getNextRequest(), 0);
        }
        private void pcHandleRoutedMessage(byte[] buff, int offset, int len, byte fromCon)
        {
            routedMessage msg = new routedMessage(buff, offset, len);

            //see if packet has reached its destination
            if (msg.Route.isForMe())//no to addresses
            {
                switch (msg.commandByte)
                {
                    case 0x01: //enumerate response
                        {
                            if (treeWalker != null)
                            {
                                treeWalker.enumerateResponse(msg);
                                routedMessage resp = treeWalker.getNextRequest();
                                if (resp != null) SendRoutedMessage(resp, 0);
                                else
                                {

                                }
                                bindtree(pcNode);
                            }
                            break;
                        }
                    case 0x91: //code update initialized
                        {
                            if (rUpload != null)
                            {
                                msg.readByte();
                                string moduleType = msg.readString();
                                string key;
                                if (activeNode.name == "TX") key = "txbinpath";
                                else key = "rxbinpath";
                                if (moduleType == "JN5148") key += "5148";

                                string filename = System.Configuration.ConfigurationManager.AppSettings.GetValues(key)[0];
                                rUpload.setFile(filename);
                                routedMessage reply = rUpload.sendNextCmd(msg);
                                if (reply != null) SendRoutedMessage(reply, fromCon);
                            }
                            break;
                        }
                    case 0x93: //code chunk recieved
                        {
                            if (rUpload != null)
                            {
                                routedMessage reply = rUpload.sendNextCmd(msg);
                                if (reply != null) SendRoutedMessage(reply, fromCon);
                            }
                            break;
                        }
                    case 0x95: //
                        {
                            msg.readByte();
                            MessageBox.Show("Upload Failed " + msg.readByte());
                            break;
                        }
                    case 0xff://debug message
                        {
                            msg.readByte();
                            SetText(msg.readString());
                            break;
                        }
                    case 0x0a://common commands
                        {
                            handleCommonCommand(msg, fromCon);
                            break;
                        }
                    case 0x04://get parameter value response
                        {
                            msg.readByte();
                            byte paramIdx = msg.readByte();
                            //depending on type read value

                          //  byte val = msg.readByte();

                            activeNode.getParamByIdx((int)paramIdx).parseValue(msg);


                            //    nodeParameterList[NodeParameterNames[paramIdx]] = val;

                            propertyGrid1.Refresh();

                            if (activeNode.parameterCount > activeNode.properties.Count)
                            {
                                //get next parameter
                                routedMessage msgGetMeta = new routedMessage(activeNode.address, new byte[] { 0x0a, 0x6, (byte)activeNode.properties.Count });
                                SendRoutedMessage(msgGetMeta, SERIALCON);
                            }
                            break;
                        }
                }
            }
            else
            {
                //todo relay using routedMessage class
                /*
                //relay message
                byte to = buff[offset + addrToIdx + 1];
                //replace to with from
                buff[offset + addrToIdx + 1] = fromCon;
                //move index of next to address
                addrToIdx++;
                buff[offset] = (byte)((buff[offset] & 0xf0) + addrToIdx);
                //pass message on to connector defined by 'to' address
                pcSendRoutedMessage(buff, offset, (byte)len, to);
                */
            }
        }
        void SendRoutedMessage(routedMessage message, byte toCon)
        {
            byte[] msg = message.toByteArray();
            pcSendRoutedMessage(msg, 0, (byte)msg.Length, toCon);
        }
        void pcSendRoutedMessage(byte[] buff, int offset, byte len, byte toCon)
        {
            switch (toCon)
            {
                case 0: JennicPacket.Write(inp, buff, offset, len, 0xff); break;
            }
        }
        private void bindtree(routedNode node)
        {
            treeView1.Nodes.Clear();
            bindtree(node, treeView1.Nodes.Add(node.name));
        }
        private void bindtree(routedNode node, TreeNode tNode)
        {
            tNode.Tag = node;
            foreach (routedNode child in node.children.Values)
            {
                TreeNode childt = tNode.Nodes.Add(child.name);
                bindtree(child, childt);
            }
        }

        private void treeView1_AfterSelect(object sender, TreeViewEventArgs e)
        {
            //populate options for node 
            //quick fix use node name to select options
            //todo enumerate options from node or cache
            routedNode node = (routedNode)e.Node.Tag;
            activeNode = node;
            labelNodeName.Text = node.name;
            switch (node.name)
            {
                case "TX":
                    buttonUploadCode.Visible = true;
                    buttonResetNode.Visible = true;
                    getNodeParameters();
                    break;
                case "RX":
                    buttonUploadCode.Visible = true;
                    buttonResetNode.Visible = true;
                    getNodeParameters();
                    break;
                default:
                    buttonUploadCode.Visible = false;
                    buttonResetNode.Visible = false;
                    nodeParameterList.Properties.Clear();
                    activeNode.properties.Clear();
                    propertyGrid1.Refresh();
                    break;
            }
        }

        private void buttonUploadCode_Click(object sender, EventArgs e)
        {
            if (activeNode.name == "TX")
            {
                string filename = System.Configuration.ConfigurationManager.AppSettings.GetValues("txbinpath")[0];
                string filename5148 = System.Configuration.ConfigurationManager.AppSettings.GetValues("txbinpath5148")[0];
                rUpload = new routedUploader();
            }
            if (activeNode.name == "RX")
            {
                string filename = System.Configuration.ConfigurationManager.AppSettings.GetValues("rxbinpath")[0];
                string filename5148 = System.Configuration.ConfigurationManager.AppSettings.GetValues("rxbinpath5148")[0];
                rUpload = new routedUploader();
            }

            routedMessage msg = new routedMessage(activeNode.address, new byte[] { 0x90 });
            SendRoutedMessage(msg, SERIALCON);
        }

        private void buttonResetNode_Click(object sender, EventArgs e)
        {
            routedMessage msg = new routedMessage(activeNode.address, new byte[] { 0x96 });
            SendRoutedMessage(msg, SERIALCON);
        }
        private void getNodeParameters()
        {
            nodeParameterList.Properties.Clear();
            activeNode.properties.Clear();

            propertyGrid1.Refresh();

            //send get parameter copunt command
            routedMessage msg = new routedMessage(activeNode.address, new byte[] { 0x0a, 0x4, 0x00 });
            SendRoutedMessage(msg, SERIALCON);

        }
        private void handleCommonCommand(routedMessage msg, byte fromCon)
        {
            msg.readByte();
            byte cmd = msg.readByte();
            switch (cmd)
            {
                case 5: //parameter count
                    byte count = msg.readByte();
                    activeNode.parameterCount = count;
                    if (count > 0)
                    {
                        routedMessage msgGetMeta = new routedMessage(activeNode.address, new byte[] { 0x0a, 0x6, 0x00 });
                        SendRoutedMessage(msgGetMeta, SERIALCON);
                    }
                    break;
                case 7: //parameter metadata
                    byte pIdx = msg.readByte();
                    string pName = msg.readString();
                    byte pType = msg.readByte();

                    ccParameter property = new ccParameter(pIdx, pName, pType);
                    activeNode.properties.Add(pName, property);

                    //bind to property editor
                    Flobbster.Windows.Forms.PropertySpec p =
                        new Flobbster.Windows.Forms.PropertySpec(pName, property.type, "Parameters");
                    nodeParameterList.Properties.Add(p);

                    propertyGrid1.Refresh();

                    //get value
                    routedMessage msgValReq = new routedMessage(activeNode.address, new byte[] { 0x03, pIdx });
                    SendRoutedMessage(msgValReq, SERIALCON);


                    break;
            }
        }
    }
}