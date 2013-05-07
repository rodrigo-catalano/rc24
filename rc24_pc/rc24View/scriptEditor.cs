using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using bluntsharp;
using System.IO;
using rc24;
using System.Runtime.InteropServices;

namespace Serial
{
    public partial class scriptEditor : UserControl
    {
        private const int EM_SETTABSTOPS = 0x00CB;

        [DllImport("User32.dll", CharSet = CharSet.Auto)]
        private static extern IntPtr SendMessage(IntPtr h, int msg, int wParam, int[] lParam);

        public static void SetTabStopWidth( TextBox textbox, int width)
        {
            SendMessage(textbox.Handle, EM_SETTABSTOPS, 1, new int[] { width * 4 });
        }

        public scriptEditor()
        {
            InitializeComponent();
            SetTabStopWidth(textBoxCode, 4);
        }

        public routedObject PC { get; set; }

        public string source
        {
            get
            {
                return textBoxCode.Text;
            }
            set
            {
                textBoxCode.Text = value;
            }
        }
        public string filename = null;

        private string newProg = @"num main(PC pc, TX tx, RX rx, Pilot pilot,IMU imu)
{

}";
        
        private string whileFunction = @"
num while(code test, code action)
{
	startw#
	@jz(interp(test),donew)
	interp(action)
	@j(startw)
	donew#
	return(0)
}";
        private string ifFunction = @"
num if(num test,code then)
{
	@jz(test,donei)
	interp(then)
	return(1)
	donei#
	return(0)
}
num if(num test,code then, code else)
{
	@jz(test,doneie)
	interp(then)
	return(1)
	doneie#
	interp(else)
	return(0)
}";
        private string forFunction = @"

num for(code init,code test,code after,code action)
{
	interp(init)
    startf#
	@jz(interp(test),donef)
	interp(action)
	interp(after)
    @j(startf)
	donef#
	return(0)
}
";
        private string loopFunction = @"
num loop(num times, code do)
{
    start#
	@jz(times,done)
	interp(do)
    times=times-1
	@j(start)
	done#
	return(0)
}";
        private void scriptEditor_Load(object sender, EventArgs e)
        {
            source = newProg;
        }
        private void buildExternalFunctions(routedObject Node, functionList functions)
        {
            if (Node == null) return;
            string prefix = Node.node.name; //use to create unique function names
            for (int i = 0; i < Node.node.parameterCount; i++)
            {
                var param = Node.node.getParamByIdx(i);
                string fName = param.Name.Replace(" ", "");

                if (param.Value != null && param.Value.GetType().IsArray)
                {
                    //function to get array object
                    

                    functions.Add(
                        new bluntFunction("get" + fName,
                            new bluntParam[] { new bluntParam(vtype.obj) { objectType = "rointarray" }, new bluntParam(vtype.obj) { objectType = prefix }, new bluntParam(vtype.constant, i) }, BluntInterp4.OP_get_routed_array));


/*
                    functions.Add(prefix + "get" + fName,
                        new bluntFunction("OP_get_routed_array_item",
                            new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.constant, i), new bluntParam(vtype.num) }, BluntInterp4.OP_get_routed_array_item));

                    functions.Add(prefix + "set" + fName,
                        new bluntFunction("OP_set_routed_array_item",
                            new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.constant, i), new bluntParam(vtype.num), new bluntParam(vtype.num) }, BluntInterp4.OP_set_routed_array_item));
                    */
                }
                else
                {
                    functions.Add(
                        new bluntFunction("get" + fName,
                            new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.obj) { objectType = prefix }, new bluntParam(vtype.constant, i) }, BluntInterp4.OP_get_routed_int16));

                 
                    functions.Add(
                        new bluntFunction("set" + fName,
                            new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.obj) { objectType = prefix }, new bluntParam(vtype.constant, i), new bluntParam(vtype.num) }, BluntInterp4.OP_set_routed_int16));//3

                }

            }
        }

        private void run(bool debug)
        {
            string fullSource = source;

            //very crude selective use of library functions
            if (source.Contains("while(")) fullSource += whileFunction;
            if (source.Contains("if(")) fullSource += ifFunction;
            if (source.Contains("for(")) fullSource += forFunction;
            if (source.Contains("loop(")) fullSource += loopFunction;

            var p = new parser5(fullSource);
          //  var p = new parser(fullSource);

            astNode tree;
            try
            {
                tree = p.parse();
            }
            catch (Exception parseErr)
            {
                MessageBox.Show("Parse Error " + parseErr.Message + " Line " + p.line);
                return;
            }

            if (debug) displaySyntaxTree(tree);
            compiler.fixDotNotation(tree);
            if (debug) displaySyntaxTree(tree);

            compiler comp = new compiler();

            //get functions from connected devices
            var ExternalFunctions = new functionList();

            dynamic RX = PC.findNode("RX");
            buildExternalFunctions(RX, ExternalFunctions);
            dynamic IMU = PC.findNode("IMU");
            buildExternalFunctions(IMU, ExternalFunctions);
            dynamic pilot = PC.findNode("Pilot");
            buildExternalFunctions(pilot, ExternalFunctions);
            dynamic TX = PC.findNode("TX");
            buildExternalFunctions(TX, ExternalFunctions);

            if (PC.node.parameterCount == 0)
            {
                PC.node.properties.Add("say", new ccParameter(0, "say", 7, 0, PC.node));
                PC.node.parameterCount++;
            }

            buildExternalFunctions(PC, ExternalFunctions);

            int[] bbc;
            try
            {
                bbc = comp.compile(tree, ExternalFunctions);
            }
            catch (Exception ce)
            {
               string msg = "Compilation Error " + ce.Message;
               foreach (var k in ce.Data.Keys) msg += " " + k.ToString() + " " + ce.Data[k].ToString();
               MessageBox.Show(msg);
               return;
           }


            if (debug)
            {
                comp.list(bbc);
                Console.Out.WriteLine(comp.listMethods());
            }

            if (ComboBoxTarget.SelectedItem.ToString() != "PC")
            {
                //run on receiver
                if (pilot != null)
                {
                    try
                    {
                        //stop existing script running
                        pilot.enabled = false;
                        //send to script array on rx
                        pilot.script = bbc;

                        //int[] test = pilot.script;

                        Console.Out.WriteLine("Script Uploaded");

                        //run code
                        pilot.enabled = true;
                    }
                    catch (Exception RXrunEx)
                    {
                        MessageBox.Show("Failed to run exception " + RXrunEx.Message);
                    }
                }
                else
                {
                    Console.Out.WriteLine("Pilot node not found");
                }
            }
            if (ComboBoxTarget.SelectedItem.ToString() == "PC")
            {
                // run script on pc virtual machine

                int[] inputs = new int[] { 0, 1, 2, 3, 4 };
                try
                {
                    comp.run(inputs, debug, new List<object> { PC, TX, RX, pilot, IMU });
                }
                catch (Exception runEx)
                {
                    MessageBox.Show("Interp exception " + runEx.Message);
                }

            }
            //     emitBBC(bbc);
        }

        public static void displaySyntaxTree(astNode n)
        {
            StringBuilder sb = new StringBuilder();
            displaySyntaxTree(n, "", sb);
            Console.Out.Write(sb.ToString());
        }

        public static void displaySyntaxTree(astNode n, String prefix, StringBuilder sb)
        {
            if (n.children != null && n.children.Count > 0)
            {

                sb.AppendLine(prefix + "  " + n.type + " " + n.name + " " + n.val);
                for (int i = 0; i < n.children.Count; i++)
                {
                    displaySyntaxTree(n.children[i], prefix + "| ", sb);
                }
                sb.AppendLine(prefix + " -" + n.type + " " + n.name + " " + n.val);

            }
            else sb.AppendLine(prefix + "  " + n.type + " " + n.name + " " + n.val);
        }

        private void newToolStripButton_Click(object sender, EventArgs e)
        {
            source = newProg;
            filename = null;
        }

        private void openToolStripButton_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = "script files (*.txt)|*.txt|All files (*.*)|*.*";
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                source = File.ReadAllText(ofd.FileName);
                filename = ofd.FileName;
            }
        }

        private void saveToolStripButton_Click(object sender, EventArgs e)
        {
            if (filename == null)
            {
                SaveFileDialog sfd = new SaveFileDialog();
                sfd.Filter = "script files (*.txt)|*.txt|All files (*.*)|*.*";
                if (sfd.ShowDialog() == DialogResult.OK)
                {
                    File.WriteAllText(sfd.FileName, source);
                    filename = sfd.FileName;
                }
            }
            else
            {
                File.WriteAllText(filename, source);
            }
        }

        private void toolStripButtonRun_Click(object sender, EventArgs e)
        {
            run(false);
        }

        private void toolStripButtonDebug_Click(object sender, EventArgs e)
        {
            run(true);
        }
    }


}
