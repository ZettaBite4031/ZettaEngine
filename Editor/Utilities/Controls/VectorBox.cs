using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;

namespace Editor.Utilities.Controls
{               
    public enum VectorType
    {
        Vec2,
        Vec3,
        Vec4
    }

    class VectorBox : Control
    {

        public Orientation Orientation
        {
            get { return (Orientation)GetValue(OrientationProperty); }
            set { SetValue(OrientationProperty, value); }
        }

        // using a DependencyProperty as the backing store for Orientation 
        // enabling animation, styling, binding, and more
        public static readonly DependencyProperty OrientationProperty =
            DependencyProperty.Register(nameof(Orientation), typeof(Orientation), typeof(VectorBox), new PropertyMetadata(Orientation.Horizontal));
                
        public VectorType VectorType
        {
            get { return (VectorType)GetValue(VectorTypeProperty); }
            set { SetValue(VectorTypeProperty, value); }
        }

        // using a DependencyProperty as the backing store for VectorType 
        // enabling animation, styling, binding, and more
        public static readonly DependencyProperty VectorTypeProperty =
            DependencyProperty.Register(nameof(VectorType), typeof(VectorType), typeof(VectorBox), new PropertyMetadata(VectorType.Vec3));
                
        public double Multiplier
        {
            get => (double)GetValue(MultiplierProperty);
            set => SetValue(MultiplierProperty, value);
        }
        public static readonly DependencyProperty MultiplierProperty =
            DependencyProperty.Register(nameof(Multiplier), typeof(double), typeof(VectorBox),
                new PropertyMetadata(1.0));

        public string X
        {
            get { return (string)GetValue(XProperty); }
            set { SetValue(XProperty, value); }
        }

        // using a DependencyProperty as the backing store for X 
        // enabling animation, styling, binding, and more
        public static readonly DependencyProperty XProperty =
            DependencyProperty.Register(nameof(X), typeof(string), typeof(VectorBox), 
                new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault));

        public string Y
        {
            get { return (string)GetValue(YProperty); }
            set { SetValue(YProperty, value); }
        }

        // using a DependencyProperty as the backing store for Y 
        // enabling animation, styling, binding, and more
        public static readonly DependencyProperty YProperty =
            DependencyProperty.Register(nameof(Y), typeof(string), typeof(VectorBox), 
                new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault));

        public string Z
        {
            get { return (string)GetValue(ZProperty); }
            set { SetValue(ZProperty, value); }
        }

        // using a DependencyProperty as the backing store for Z 
        // enabling animation, styling, binding, and more
        public static readonly DependencyProperty ZProperty =
            DependencyProperty.Register(nameof(Z), typeof(string), typeof(VectorBox), 
                new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault));


        public string W
        {
            get { return (string)GetValue(WProperty); }
            set { SetValue(WProperty, value); }
        }

        // using a DependencyProperty as the backing store for W 
        // enabling animation, styling, binding, and more
        public static readonly DependencyProperty WProperty =
            DependencyProperty.Register(nameof(W), typeof(string), typeof(VectorBox),
                new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault));
                

        static VectorBox()
        {
            DefaultStyleKeyProperty.OverrideMetadata(typeof(VectorBox),
                new FrameworkPropertyMetadata(typeof(VectorBox)));
        }
    }
}
