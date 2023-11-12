using Editor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Editor.Content
{
    enum PrimitiveMeshType 
    {
        Plane,
        Cube,
        UVSphere,
        Icosphere,
        Cylinder,
        Capsule
    }

    class Mesh : ViewModelBase
    {
        private int _VertexSize;
        public int VertexSize
        {
            get => _VertexSize;
            set
            {
                if (_VertexSize != value)
                {
                    _VertexSize = value;
                    OnPropertyChanged(nameof(VertexSize));
                }
            }
        }

        private int _VertexCount;
        public int VertexCount
        {
            get => _VertexCount;
            set
            {
                if (_VertexCount != value)
                {
                    _VertexCount = value;
                    OnPropertyChanged(nameof(VertexCount));
                }
            }
        }

        private int _IndexSize;
        public int IndexSize
        {
            get => _IndexSize;
            set
            {
                if (_IndexSize != value)
                {
                    _IndexSize = value;
                    OnPropertyChanged(nameof(IndexSize));
                }
            }
        }

        private int _IndexCount;
        public int IndexCount
        {
            get => _IndexCount;
            set
            {
                if (_IndexCount != value)
                {
                    _IndexCount = value;
                    OnPropertyChanged(nameof(IndexCount));
                }
            }
        }

        public byte[] Vertices { get; set; }
        public byte[] Indices { get; set; }
    }

    class MeshLOD : ViewModelBase
    {
        private string _Name;
        public string Name
        {
            get => _Name;
            set
            {
                if (_Name != value)
                {
                    _Name = value;
                    OnPropertyChanged(nameof(Name));
                }
            }
        }

        private float _LODThreshold;
        public float LODThreshold
        {
            get => _LODThreshold;
            set
            {
                if (_LODThreshold != value)
                {
                    _LODThreshold = value;
                    OnPropertyChanged(nameof(LODThreshold));
                }
            }
        }

        public ObservableCollection<Mesh> Meshes { get; } = new ObservableCollection<Mesh>();
    }

    class LODGroup : ViewModelBase
    {

        private string _Name;
        public string Name
        {
            get => _Name;
            set
            {
                if (_Name != value)
                {
                    _Name = value;
                    OnPropertyChanged(nameof(Name));
                }
            }
        }

        public ObservableCollection<MeshLOD> LODs { get; } = new ObservableCollection<MeshLOD>();
    }

    class Geometry : Asset
    {
        private readonly List<LODGroup> _lodGroups = new List<LODGroup>();

        public LODGroup GetLODGroup(int lodGroup = 0)
        {
            Debug.Assert(lodGroup >= 0 && lodGroup < _lodGroups.Count);
            return _lodGroups.Any() ? _lodGroups[lodGroup] : null;
        }

        public void FromRawData(byte[] data)
        {
            Debug.Assert(data?.Length > 0);
            _lodGroups.Clear();

            using var reader = new BinaryReader(new MemoryStream(data));
            // skip scene name
            var s = reader.ReadInt32();
            reader.BaseStream.Position += s;
            // number of LODs
            var numLodGroups = reader.ReadInt32();
            Debug.Assert(numLodGroups > 0);

            for (int i =0; i < numLodGroups; i++)
            {
                // get LOD groups
                s = reader.ReadInt32();
                string lodGroupName;
                if (s > 0)
                {
                    var nameBytes = reader.ReadBytes(s);
                    lodGroupName = Encoding.UTF8.GetString(nameBytes);
                }
                else lodGroupName = $"lod_{ContentHelper.GetRandomString()}";
                
                // get number of meshes in this LOD group
                var numMeshes = reader.ReadInt32();
                Debug.Assert(numMeshes > 0);
                var lods = ReadMeshLODs(numMeshes, reader);

                var lodGroup = new LODGroup() { Name = lodGroupName };
                lods.ForEach(l => lodGroup.LODs.Add(l));

                _lodGroups.Add(lodGroup);
            }
        }

        private List<MeshLOD> ReadMeshLODs(int numMeshes, BinaryReader reader)
        {
            var lodIDs = new List<int>();
            var lodList = new List<MeshLOD>();
            for (int i = 0; i < numMeshes; i++) ReadMeshes(reader, lodIDs, lodList);
            return lodList;
        }

        private void ReadMeshes(BinaryReader reader, List<int> lodIDs, List<MeshLOD> lodList)
        {
            // get mesh name
            var s = reader.ReadInt32();
            string meshName;
            if (s > 0)
            {
                var nameBytes = reader.ReadBytes(s);
                meshName = Encoding.UTF8.GetString(nameBytes);
            }
            else meshName = $"mesh_{ContentHelper.GetRandomString(s)}";

            var mesh = new Mesh();

            var lodID = reader.ReadInt32();
            mesh.VertexSize = reader.ReadInt32();
            mesh.VertexCount = reader.ReadInt32();
            mesh.IndexSize = reader.ReadInt32();
            mesh.IndexCount = reader.ReadInt32();
            var lodThreshold = reader.ReadSingle();

            var vertexBufferSize = mesh.VertexSize * mesh.VertexCount;
            var indexBufferSize = mesh.IndexSize * mesh.IndexCount;

            mesh.Vertices = reader.ReadBytes(vertexBufferSize);
            mesh.Indices = reader.ReadBytes(indexBufferSize);

            MeshLOD lod;
            if (ID.IsValid(lodID) && lodIDs.Contains(lodID))
            {
                lod = lodList[lodIDs.IndexOf(lodID)];
                Debug.Assert(lod != null);
            }
            else
            {
                lodIDs.Add(lodID);
                lod = new MeshLOD() { Name = meshName, LODThreshold = lodThreshold};
                lodList.Add(lod);
            }

            lod.Meshes.Add(mesh);
        }

        public Geometry() : base(AssetType.Mesh) { }
    }
}
