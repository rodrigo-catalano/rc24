using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Dynamic;

namespace rc24
{
    // dynamic wrapper of routedNode to allow code like PC.TX.Demands[3]=10;
    // speed is not an issue here as serial and radio comms are so slow in PC terms
    public class routedObject : DynamicObject
    {
        public static int defaultTimeout = 2000;
                        
        public routedNode node{get;set;}
        public routedConnector con { get; set; }

        public routedObject findNode(string name)
        {
            routedNode n=node.findNode(name);
            if(n!=null) return new routedObject() { node = n, con = con };
            else return null;
        }

        public override bool TryGetMember(GetMemberBinder binder, out object result)
        {
            //look for child nodes
            foreach (var n in node.children.Values)
            {
                if (n.name == binder.Name)
                {
                    result = new routedObject() { node = n,con=con};
                    return true;
                }
            }
            //look for field
            foreach (var p in node.properties.Keys)
            {
                if (p.Replace(" ", "") == binder.Name)
                {
                    //send request message
                    result = getProperty(node.properties[p],0,0);
                   return true;
                }
            }
            
            return base.TryGetMember(binder, out result);
        }
        public object getProperty(ccParameter param,int start,int num)
        {
            //send request message
            
            routedMessage m = new routedMessage(node.address, new byte[] { 0x03, (byte)param.Index, (byte)start, (byte)num });
            MessageSubscription sub = new MessageSubscription() { singleShot = true, sent = m };
                   
            con.SendRoutedMessage(m, 0, sub);
            //wait for reply
            asyncWaitForReply(sub, defaultTimeout);
                        
            //decode message
            param.parseValue(sub.reply);

            return param.Value;
          
        }
        public int getArrayItemAsInt(int paramIdx,int arrayIdx)
        {
            ccParameter param=node.getParamByIdx(paramIdx);
            getProperty(param, arrayIdx, 1);

            //TODO make work for all array types
            switch (param.TypeIdx)
            {
                case ccParameter.CC_UINT8_ARRAY: return (int)((byte[])param.Value)[arrayIdx];
                case ccParameter.CC_INT8_ARRAY: return (int)((sbyte[])param.Value)[arrayIdx];
                case ccParameter.CC_UINT16_ARRAY: return (int)((UInt16[])param.Value)[arrayIdx];
                case ccParameter.CC_INT16_ARRAY: return (int)((Int16[])param.Value)[arrayIdx];
                case ccParameter.CC_UINT32_ARRAY: return (int)((UInt32[])param.Value)[arrayIdx];
                case ccParameter.CC_INT32_ARRAY: return (int)((Int32[])param.Value)[arrayIdx];
                default: throw new Exception(" routed object getArrayItemFromInt type not handled " + param.TypeIdx);
            }
         }
        public void setArrayItemFromInt(int paramIdx, int arrayIdx,int value)
        {
            ccParameter param = node.getParamByIdx(paramIdx);

            switch(param.TypeIdx)
            {
                case ccParameter.CC_UINT8_ARRAY: ((byte[])param.Value)[arrayIdx] = (byte)value; break;
                case ccParameter.CC_INT8_ARRAY: ((sbyte[])param.Value)[arrayIdx] = (sbyte)value; break;
                case ccParameter.CC_UINT16_ARRAY: ((UInt16[])param.Value)[arrayIdx] = (UInt16)value; break;
                case ccParameter.CC_INT16_ARRAY : ((Int16[])param.Value)[arrayIdx] = (Int16)value; break;
                case ccParameter.CC_UINT32_ARRAY : ((UInt32[])param.Value)[arrayIdx] = (UInt32)value; break;
                case ccParameter.CC_INT32_ARRAY : ((Int32[])param.Value)[arrayIdx] = (Int32)value; break;
                default: throw new Exception(" routed object setArrayItemFromInt type not handled " + param.TypeIdx);         
            }
            setProperty(param, arrayIdx, 1);
        }
        public override bool TrySetMember(SetMemberBinder binder, object value)
        {
            //look for field
            foreach (var p in node.properties.Keys)
            {
                if (p.Replace(" ","") == binder.Name)
                {
                    ccParameter param=node.properties[p];
                    
                    int len = 0;
                    if (param.Value.GetType().IsArray)
                    {
                        len=Math.Min(((Array)value).Length,((Array)param.Value).Length);
                        System.Array.Copy((Array)value,(Array)param.Value,len);
                    }
                    else param.Value = value;
                    setProperty(param, 0, len);

                    return true;
                }
            }
            
            return false;
        }
        public void setProperty(ccParameter param, int start, int num)
        {
            //assume value already set in local param
            uint nBlocks = 1;
            uint blockSize = 0;
            if (param.Value.GetType().IsArray)
            {
                if (start >= param.ArrayLen) throw (new Exception("Set routed Property index too big"));
                num = Math.Min(num, (int)param.ArrayLen - start);
                //calc number of blocks to send
                blockSize = 32;
                nBlocks = ((uint)num - 1) / blockSize + 1;
            }
            
            UInt32 startidx = (UInt32)start;
            for (int i = 0; i < nBlocks; i++)
            {
                byte size = (byte)Math.Min(blockSize, start+num - startidx);
                
                routedMessage m = new routedMessage(node.address, param.buildSetCmd(startidx, size));
                MessageSubscription sub = new MessageSubscription() { singleShot = true, sent = m };

                con.SendRoutedMessage(m, 0, sub);
                //wait for ack
                asyncWaitForReply(sub, defaultTimeout);
                //TODO check ack?
                startidx += blockSize;
                
            }
        }
        private void asyncWaitForReply(MessageSubscription sub,int timeout)
        {
            // bit of a bodge but makes code so much simpler
            // for .net4.5 replace with async method
            DateTime start = DateTime.Now;
            while (sub.reply == null)
            {
                System.Windows.Forms.Application.DoEvents();

                if ((DateTime.Now - start).TotalMilliseconds >= timeout)
                {
                    if(sub.singleShot)sub.expired = true;
                    throw new Exception("Routed Object reply timeout");
                }
            }
        }
    }
}
