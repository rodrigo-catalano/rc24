using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace rc24
{
    public class MessageSubscription
    {
        public bool singleShot { get; set; }
       // private Task<routedMessage> t;
        public routedMessage reply;
        public routedMessage sent;
        public bool expired = false;
        public void handleReply(routedMessage message)
        {
            //pass to async method
            reply = message;
        }

        public bool matches(routedMessage message)
        {
            //crudely matches set and get responses
            
            if (message.Command[0] == sent.Command[0] + 1)
            {
                // TODO for get commands match more for set commands, ack needs more info   
                return true;
            }
            return false;
        }
        /*
        public int GetIntAsync(int add, int index)
        {
            //send message with subscription

            //wait for reply
            while (reply == null)
            {
                 System.Windows.Forms.Application.DoEvents();
            }
            //decode message
        }
        */
        /*
        
        static public Task<int> GetIntAsync(int add,int index)
        {
            MessageSubscription sub=new MessageSubscription();
            sub.singleShot=true;
            //send message
         
            
            //wait for response
            
            //decode message
            
            return 1;
        }
        public Task<routedMessage> awaitReply()
        {
            //a task that can be waited on and is signalled by subscribed messages
        }
         * */
    }
     
    
    

}
