using System;
using System.Collections.Generic;
using System.Collections;
using System.Text;
using Flobbster.Windows.Forms;
using rc24;

namespace Serial
{
    public class CC_paramBinder : PropertyBag
    {
        private routedNode _node=null;

        /// <summary>
        /// Initializes a new instance of the PropertyTable class.
        /// </summary>
        public CC_paramBinder(routedNode node)
        {
            Node = node;
        }
        public CC_paramBinder()
        {
        }
        public routedNode Node
        {
            set
            {
                _node = value;
                Properties.Clear();
            }
            get
            {
                return _node;
            }
        }
                        
        protected override void OnSetValue(PropertySpecEventArgs e)
        {
            string paramName = e.Property.Name;

            ccParameter param = Node.properties[paramName];
            param.Value = e.Value;
            base.OnSetValue(e);
        }

        protected override void OnGetValue(PropertySpecEventArgs e)
        {
            e.Value = Node.properties[e.Property.Name].Value;
        }
    }
}
