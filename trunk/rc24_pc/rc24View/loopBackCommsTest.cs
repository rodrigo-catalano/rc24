using System;
using System.Collections.Generic;
using System.Text;
using rc24;
using System.Windows.Forms;

namespace Serial
{
    public class loopBackCommsTest
    {
        private int nTries = 0;
        byte[] message;
        Random rand;
        route Route;
        public int lengtherrors = 0;
        public int contenterrors = 0;
        public loopBackCommsTest(int n, route r)
        {
            nTries = n;
            rand = new Random();
            Route = r;
        }
        public routedMessage sendNextCmd(routedMessage msg)
        {
            if (msg != null)
            {
                //check response
                if (msg.Command.Count != message.Length)
                {
                    MessageBox.Show("length error " + msg.Command.Count + " " + message.Length);
                    lengtherrors++;
                }
                else
                {
                    bool ok = true;
                    for (int i = 0; i < message.Length; i++)
                    {
                        if (message[i] != msg.Command[i]) ok = false;
                    }
                    if(!ok)
                    {
                        contenterrors++;
                        MessageBox.Show("content error");
                    }
                }

            }
            if (nTries-- > 0)
            {
                int n = rand.Next() % 192+4;
                message = new byte[n];
                rand.NextBytes(message);
                message[0] = 0xfe;
                return new routedMessage(Route, message);
            }
            else return null;
        }
    }
}
