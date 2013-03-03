using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace Serial
{
    public class TextBoxStreamWriter : TextWriter
    {
        TextBox _output = null;
        public TextBoxStreamWriter(TextBox output)
        {
            _output = output;
        }
        public override void Write(char value)
        {
            base.Write(value);
            _output.AppendText(value.ToString());
        }
        public override void WriteLine(string value)
        {
            _output.AppendText(value);
            _output.AppendText("\r\n");

        }
        public override void Write(string value)
        {
            // base.Write(value);
         //   _output.AppendText(value);

            _output.SelectionStart = _output.TextLength;
            _output.SelectedText = value;
 
 

        }

        public override Encoding Encoding
        {
            get { return System.Text.Encoding.UTF8; }

        }
    }
}
