using Editor.Utilities;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Numerics;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace Editor.Components
{
    [DataContract]
    class Transform : Component
    {
        private Vector3 _Position;
        [DataMember]
        public Vector3 Position
        {
            get => _Position;
            set
            {
                if (_Position != value)
                {
                    _Position = value;
                    OnPropertyChanged(nameof(Position));
                }
            }
        }

        private Vector3 _Rotation;
        [DataMember]
        public Vector3 Rotation
        {
            get => _Rotation;
            set
            {
                if (_Rotation != value)
                {
                    _Rotation = value;
                    OnPropertyChanged(nameof(Rotation));
                }
            }
        }

        private Vector3 _Scale;
        [DataMember]
        public Vector3 Scale
        {
            get => _Scale;
            set
            {
                if (_Scale != value)
                {
                    _Scale = value;
                    OnPropertyChanged(nameof(Scale));
                }
            }
        }

        public override IMSComponent GetMSComponent(MSEntity msEntity) => new MSTransform(msEntity);

        public override void WriteToBinary(BinaryWriter bw)
        {
            bw.Write(_Position.X); bw.Write(_Position.Y); bw.Write(_Position.Z);
            bw.Write(_Rotation.X); bw.Write(_Rotation.Y); bw.Write(_Rotation.Z);
            bw.Write(_Scale.X);    bw.Write(_Scale.Y);    bw.Write(_Scale.Z);
        }

        public Transform(GameEntity owner) : base(owner) {}
    }

    sealed class MSTransform : MSComponent<Transform>
    {

        private float? _PosX;
        public float? PosX
        {
            get => _PosX;
            set
            {
                if (!_PosX.IsEquals(value))
                {
                    _PosX = value;
                    OnPropertyChanged(nameof(PosX));
                }
            }
        }

        private float? _PosY;
        public float? PosY
        {
            get => _PosY;
            set
            {
                if (!_PosY.IsEquals(value))
                {
                    _PosY = value;
                    OnPropertyChanged(nameof(PosY));
                }
            }
        }

        private float? _PosZ;
        public float? PosZ
        {
            get => _PosZ;
            set
            {
                if (!_PosZ.IsEquals(value))
                {
                    _PosZ = value;
                    OnPropertyChanged(nameof(PosZ));
                }
            }
        }

        private float? _RotX;
        public float? RotX
        {
            get => _RotX;
            set
            {
                if (!_RotX.IsEquals(value))
                {
                    _RotX = value;
                    OnPropertyChanged(nameof(RotX));
                }
            }
        }

        private float? _RotY;
        public float? RotY
        {
            get => _RotY;
            set
            {
                if (!_RotY.IsEquals(value))
                {
                    _RotY = value;
                    OnPropertyChanged(nameof(RotY));
                }
            }
        }

        private float? _RotZ;
        public float? RotZ
        {
            get => _RotZ;
            set
            {
                if (!_RotZ.IsEquals(value))
                {
                    _RotZ = value;
                    OnPropertyChanged(nameof(RotZ));
                }
            }
        }

        private float? _ScaleX;
        public float? ScaleX
        {
            get => _ScaleX;
            set
            {
                if (!_ScaleX.IsEquals(value))
                {
                    _ScaleX = value;
                    OnPropertyChanged(nameof(ScaleX));
                }
            }
        }

        private float? _ScaleY;
        public float? ScaleY
        {
            get => _ScaleY;
            set
            {
                if (!_ScaleY.IsEquals(value))
                {
                    _ScaleY = value;
                    OnPropertyChanged(nameof(ScaleY));
                }
            }
        }

        private float? _ScaleZ;
        public float? ScaleZ
        {
            get => _ScaleZ;
            set
            {
                if (!_ScaleZ.IsEquals(value))
                {
                    _ScaleZ = value;
                    OnPropertyChanged(nameof(ScaleZ));
                }
            }
        }

        protected override bool UpdateComponents(string propertyName)
        {
            switch(propertyName)
            {
                case nameof(PosX):
                case nameof(PosY):
                case nameof(PosZ):
                    SelectedComponents.ForEach(c => c.Position = new Vector3(_PosX ?? c.Position.X, _PosY ?? c.Position.Y, _PosZ ?? c.Position.Z));
                    return true;

                case nameof(RotX):
                case nameof(RotY):
                case nameof(RotZ):
                    SelectedComponents.ForEach(c => c.Rotation = new Vector3(_RotX ?? c.Rotation.X, _RotY ?? c.Rotation.Y, _RotZ ?? c.Rotation.Z));
                    return true;

                case nameof(ScaleX):
                case nameof(ScaleY):
                case nameof(ScaleZ):
                    SelectedComponents.ForEach(c => c.Scale = new Vector3(_ScaleX ?? c.Scale.X, _ScaleY ?? c.Scale.Y, _ScaleZ ?? c.Scale.Z));
                    return true;
            }
            return false;
        }

        protected override bool UpdateMSComponent()
        {
            PosX = MSEntity.GetMixedValues(SelectedComponents, new Func<Transform, float>(x => x.Position.X));
            PosY = MSEntity.GetMixedValues(SelectedComponents, new Func<Transform, float>(x => x.Position.Y));
            PosZ = MSEntity.GetMixedValues(SelectedComponents, new Func<Transform, float>(x => x.Position.Z));

            RotX = MSEntity.GetMixedValues(SelectedComponents, new Func<Transform, float>(x => x.Rotation.X));
            RotY = MSEntity.GetMixedValues(SelectedComponents, new Func<Transform, float>(x => x.Rotation.Y));
            RotZ = MSEntity.GetMixedValues(SelectedComponents, new Func<Transform, float>(x => x.Rotation.Z));

            ScaleX = MSEntity.GetMixedValues(SelectedComponents, new Func<Transform, float>(x => x.Scale.X));
            ScaleY = MSEntity.GetMixedValues(SelectedComponents, new Func<Transform, float>(x => x.Scale.Y));
            ScaleZ = MSEntity.GetMixedValues(SelectedComponents, new Func<Transform, float>(x => x.Scale.Z));

            return true;
        }

        public MSTransform(MSEntity msEntity) : base(msEntity)
        {
            Refresh();
        }
    }
}
