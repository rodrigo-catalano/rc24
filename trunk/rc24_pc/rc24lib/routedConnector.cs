using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace rc24
{
    public interface routedConnector
    {
        void SendRoutedMessage(routedMessage message, byte toCon, MessageSubscription reply);
    }
}
