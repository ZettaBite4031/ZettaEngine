﻿using Editor.ContentToolsAPIStructs;
using Editor.DLLWrapper;
using Editor.Editors;
using Editor.GameProject;
using Editor.Utilities.Controls;
using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace Editor.Content
{
    /// <summary>
    /// Interaction logic for PrimitiveMeshDialog.xaml
    /// </summary>
    public partial class PrimitiveMeshDialog : Window
    {
        private static readonly List<ImageBrush> _textures = new();    

        private void OnPrimitiveType_ComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e) => UpdatePrimitive();

        private void OnSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e) => UpdatePrimitive();

        private void OnScalarBox_ValueChanged(object sender, RoutedEventArgs e) => UpdatePrimitive();

        private float Value(ScalarBox scalarBox, float min)
        {
            float.TryParse(scalarBox.Value, out var value);
            return Math.Max(value, min);
        }

        private void UpdatePrimitive()
        {
            if (!IsInitialized) return;

            var primitiveType = (PrimitiveMeshType)primitiveTypeComboBox.SelectedItem;
            var info = new PrimitiveInitInfo() { Type = primitiveType };
            var smoothingAngle = 0;

            switch (primitiveType)
            {
                case PrimitiveMeshType.Plane:
                    info.SegmentX = (int)xSliderPlane.Value;
                    info.SegmentZ = (int)zSliderPlane.Value;
                    info.Size.X = Value(widthScalaraBoxPlane, 0.001f);
                    info.Size.Z = Value(lengthScalaraBoxPlane, 0.001f);
                    break;
                case PrimitiveMeshType.Cube:
                    return;
                    
                case PrimitiveMeshType.UVSphere:
                    info.SegmentX = (int)xSliderUVSphere.Value;
                    info.SegmentY = (int)ySliderUVSphere.Value;
                    info.Size.X = Value(xScalaraBoxUVSphere, 0.001f);
                    info.Size.Y = Value(yScalaraBoxUVSphere, 0.001f);
                    info.Size.Z = Value(zScalaraBoxUVSphere, 0.001f);
                    smoothingAngle = (int)angleSliderUVSphere.Value;
                    break;
                    
                case PrimitiveMeshType.Icosphere:
                    return;
                    
                case PrimitiveMeshType.Cylinder:
                    return;
                    
                case PrimitiveMeshType.Capsule:
                    return;

                default:
                    break;
            }

            var geometry = new Geometry();
            geometry.ImportSettings.SmoothingAngle = smoothingAngle;
            ContentToolsAPI.CreatePrimitiveMesh(geometry, info);
            (DataContext as GeometryEditor).SetAsset(geometry);
            OnTexture_CheckBox_Click(textureCheckBox, null);
        }

        private static void LoadTextures()
        {
            var uris = new List<Uri>
            {
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/PlaneTexture.png"),
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/PlaneTexture.png"),
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/Checkmap.png"),
            };

            _textures.Clear();
            foreach (var uri in uris)
            {
                var resource = Application.GetResourceStream(uri);
                using var reader = new BinaryReader(resource.Stream);
                var data = reader.ReadBytes((int)resource.Stream.Length);
                var imageSource = (BitmapSource)new ImageSourceConverter().ConvertFrom(data);
                imageSource.Freeze();
                var brush = new ImageBrush(imageSource)
                {
                    Transform = new ScaleTransform(1, -1, 0.5, 0.5),
                    ViewportUnits = BrushMappingMode.Absolute
                };
                brush.Freeze();
                _textures.Add(brush);
            }
        }

        static PrimitiveMeshDialog()
        {
            LoadTextures();
        }

        public PrimitiveMeshDialog()
        {
            InitializeComponent();
            Loaded += (s, e) => UpdatePrimitive();
        }

        private void OnTexture_CheckBox_Click(object sender, RoutedEventArgs e)
        {
            Brush brush = Brushes.White;
            if ((sender as CheckBox).IsChecked == true)
                brush = _textures[(int)primitiveTypeComboBox.SelectedItem];

            var vm = DataContext as GeometryEditor;
            foreach (var mesh in vm.MeshRenderer.Meshes)
                mesh.Diffuse = brush;
        }

        private void OnSave_Button_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new SaveDialog();
            if (dlg.ShowDialog() == true)
            {
                Debug.Assert(!string.IsNullOrEmpty(dlg.SaveFilePath));
                var asset = (DataContext as IAssetEditor).Asset;
                Debug.Assert(asset != null);
                asset.Save(dlg.SaveFilePath);
            }
        }
    }
}
