using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading.Tasks.Dataflow;
using System.Windows;
using System.Windows.Media;

namespace Editor
{
    static class VisualExtensions
    {
        public static T FindVisualParent<T>(this DependencyObject dpo) where T : DependencyObject
        {
            if (!(dpo is Visual)) return null;
           
            var parent = VisualTreeHelper.GetParent(dpo);
            while (parent != null)
            {
                if (parent is T type) return type;
                parent = VisualTreeHelper.GetParent(parent);
            }
            return null;
        }
    }

    public static class ContentHelper
    {
        public static string GetRandomString(int length = 8)
        {
            if (length <= 0) length = 8;
            var n = length / 11;
            var sb = new StringBuilder();
            for (int i = 0; i <= n; i++) sb.Append(Path.GetRandomFileName().Replace(".","")); 
            return sb.ToString(0, length);
        }
    }
}
