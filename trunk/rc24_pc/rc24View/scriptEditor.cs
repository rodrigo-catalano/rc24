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

namespace Serial
{
    public partial class scriptEditor : UserControl
    {
        public scriptEditor()
        {
            InitializeComponent();
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

        private string newProg = @"num main(TX tx,RX rx, pilot pilot,IMU imu)
{

}";

        private string whileFunction = @"
num while(code test, code action)
{
	startw:
	@jz(interp(test),donew)
	interp(action)
	@j(startw)
	donew:
	return(0)
}";
        private string ifFunction = @"
num if(num test,code action)
{
	@jz(test,donei)
	interp(action)
	return(1)
	donei:
	return(0)
}";
       private string ifelseFunction = @"

num ifelse(num test,code action, code else)
{
	@jz(test,doneie)
	interp(action)
	return(1)
	doneie:
	interp(else)
	return(0)
}";
       private string forFunction = @"

num for(code init,code test,code after,code action)
{
	interp(init)
    startf:
	@jz(interp(test),donef)
	interp(action)
	interp(after)
    @j(startf)
	donef:
	return(0)
}
";
        
        private void scriptEditor_Load(object sender, EventArgs e)
        {
            source = newProg;
        }
        private void buildExternalFunctions(routedObject Node,Dictionary<String, bluntFunction> functions)
        {
            if (Node == null) return;
            string prefix = Node.node.name; //use to create unique function names
            for( int i=0;i<Node.node.parameterCount;i++)
            {
                var param=Node.node.getParamByIdx(i);
                string fName = param.Name.Replace(" ", "");

                if (param.Value!=null && param.Value.GetType().IsArray)
                {
                    functions.Add(prefix+"get" + fName,
                        new bluntFunction("OP_get_routed_array_item",
                            new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.constant, i), new bluntParam(vtype.num) }, BluntInterp4.OP_get_routed_array_item));

                    functions.Add(prefix + "set" + fName,
                        new bluntFunction("OP_set_routed_array_item",
                            new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.constant, i), new bluntParam(vtype.num),new bluntParam(vtype.num) }, BluntInterp4.OP_set_routed_array_item));

                }
                else
                {
                    functions.Add(prefix + "get" + fName,
                        new bluntFunction("OP_get_routed_int16",
                            new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.constant, i) }, BluntInterp4.OP_get_routed_int16));

                    functions.Add(prefix + "set" + fName,
                        new bluntFunction("OP_set_routed_int16",
                            new bluntParam[] { new bluntParam(vtype.num), new bluntParam(vtype.constant, i), new bluntParam(vtype.num) }, BluntInterp4.OP_set_routed_int16));//3

                }
            
            }
        }

        private void run(bool debug)
        {
            string fullSource=source;

            //very crude selective use of libary functions
            if(source.Contains("while("))fullSource=whileFunction+fullSource;
            if(source.Contains("if(")) fullSource = ifFunction + fullSource;
            if(source.Contains("ifelse(")) fullSource = ifelseFunction + fullSource;
            if(source.Contains("for(")) fullSource = forFunction + fullSource;

            parser p = new parser(fullSource);
         
            astNode tree;
            try
            {
                tree = p.parse();
            }
            catch (Exception parseErr)
            {
                MessageBox.Show("Parse Error " + parseErr.Message + " Line" + p.line);
                return;
            }

            if(debug)displaySyntaxTree(tree);
            compiler comp = new compiler();

            //get functions from connected devices
            Dictionary<String, bluntFunction> ExternalFunctions = new Dictionary<String, bluntFunction>();

            dynamic RX = PC.findNode("RX");
            buildExternalFunctions(RX, ExternalFunctions);
            dynamic IMU = PC.findNode("IMU");
            buildExternalFunctions(IMU, ExternalFunctions);
            dynamic pilot = PC.findNode("Pilot");
       //     buildExternalFunctions(pilot, ExternalFunctions);
            dynamic TX = PC.findNode("TX");
            buildExternalFunctions(TX, ExternalFunctions);
            
       
            int[] bbc;
            try
            {
                bbc = comp.compile(tree, ExternalFunctions);
            }
            catch (Exception ce)
            {
                MessageBox.Show("Compilation Error " + ce.Message);
                return;
            }


            if(debug)comp.list(bbc);
            if (ComboBoxTarget.SelectedItem != "PC")
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
            if (ComboBoxTarget.SelectedItem == "PC")
            {
                // run script on pc virtual machine

                int[] inputs = new int[] { 0, 1,2,3 };
                try
                {
                    comp.run(inputs, debug,new object[]{TX,RX,pilot,IMU});
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
            StringBuilder sb=new StringBuilder();
            displaySyntaxTree(n, "",sb);
            Console.Out.Write(sb.ToString());
        }
        
        public static void displaySyntaxTree(astNode n, String prefix,StringBuilder sb)
        {
            if (n.children != null && n.children.Count > 0)
            {

                sb.AppendLine(prefix + "  " + n.type + " " + n.name + " " + n.val);
                for (int i = 0; i < n.children.Count; i++)
                {
                    displaySyntaxTree(n.children[i], prefix + "| ",sb);
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
