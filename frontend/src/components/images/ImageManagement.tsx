import { useState } from 'react'
import {
  useImages,
  usePullImage,
  useRemoveImage,
  useTagImage,
  useLoadImage,
  useSaveImage,
  useBuildImage
} from '@/hooks/useApi'
import { Card, CardContent, CardDescription, CardHeader, CardTitle, Button, Table, TableBody, TableCell, TableHead, TableHeader, TableRow, Input, MetaFilePicker } from '@/components/ui'
import { Badge, Alert, AlertDescription } from '@/components/ui'
import { HardDrive, Download, Trash2, Tag, Upload, Save, Hammer, Search, RefreshCw, Plus, X } from 'lucide-react'
import type { Image as ImageType } from '@/types/api'

const ImageManagement = () => {
  const [searchTerm, setSearchTerm] = useState('')
  const [showPullDialog, setShowPullDialog] = useState(false)
  const [showTagDialog, setShowTagDialog] = useState(false)
  const [showLoadDialog, setShowLoadDialog] = useState(false)
  const [showSaveDialog, setShowSaveDialog] = useState(false)
  const [showBuildDialog, setShowBuildDialog] = useState(false)
  const [showLoadFilePicker, setShowLoadFilePicker] = useState(false)
  const [showSaveFilePicker, setShowSaveFilePicker] = useState(false)
  const [showDockerfilePicker, setShowDockerfilePicker] = useState(false)
  const [showBuildContextPicker, setShowBuildContextPicker] = useState(false)
  const [selectedImage, setSelectedImage] = useState<ImageType | null>(null)
  
  // Form states
  const [pullImageName, setPullImageName] = useState('')
  const [tagSource, setTagSource] = useState('')
  const [tagTarget, setTagTarget] = useState('')
  const [loadFilePath, setLoadFilePath] = useState('')
  const [saveImageName, setSaveImageName] = useState('')
  const [saveFilePath, setSaveFilePath] = useState('')
  const [buildDockerfilePath, setBuildDockerfilePath] = useState('')
  const [buildImageName, setBuildImageName] = useState('')
  const [buildContext, setBuildContext] = useState('')

  const { data: images, refetch, isLoading } = useImages()
  const pullImage = usePullImage()
  const removeImage = useRemoveImage()
  const tagImage = useTagImage()
  const loadImage = useLoadImage()
  const saveImage = useSaveImage()
  const buildImage = useBuildImage()

  const handlePullImage = async () => {
    if (!pullImageName.trim()) return
    try {
      await pullImage.mutateAsync({ image: pullImageName })
      setPullImageName('')
      setShowPullDialog(false)
      refetch()
    } catch (error) {
      console.error('Pull failed:', error)
    }
  }

  const handleRemoveImage = async (imageId: string, force: boolean = false) => {
    if (confirm('Are you sure you want to remove this image?')) {
      try {
        await removeImage.mutateAsync({ imageId, force })
        refetch()
      } catch (error) {
        console.error('Remove failed:', error)
      }
    }
  }

  const handleTagImage = async () => {
    if (!tagSource.trim() || !tagTarget.trim()) return
    try {
      await tagImage.mutateAsync({ 
        source_image: tagSource, 
        target_tag: tagTarget 
      })
      setTagSource('')
      setTagTarget('')
      setShowTagDialog(false)
      refetch()
    } catch (error) {
      console.error('Tag failed:', error)
    }
  }

  const handleLoadImage = async () => {
    if (!loadFilePath.trim()) return
    try {
      await loadImage.mutateAsync({ file_path: loadFilePath })
      setLoadFilePath('')
      setShowLoadDialog(false)
      refetch()
    } catch (error) {
      console.error('Load failed:', error)
    }
  }

  const handleSaveImage = async () => {
    if (!saveImageName.trim() || !saveFilePath.trim()) return
    try {
      await saveImage.mutateAsync({ 
        image_name: saveImageName, 
        file_path: saveFilePath 
      })
      setSaveImageName('')
      setSaveFilePath('')
      setShowSaveDialog(false)
    } catch (error) {
      console.error('Save failed:', error)
    }
  }

  const handleBuildImage = async () => {
    if (!buildDockerfilePath.trim() || !buildImageName.trim() || !buildContext.trim()) return
    try {
      await buildImage.mutateAsync({
        dockerfile_path: buildDockerfilePath,
        image_name: buildImageName,
        build_context: buildContext
      })
      setBuildDockerfilePath('')
      setBuildImageName('')
      setBuildContext('')
      setShowBuildDialog(false)
      refetch()
    } catch (error) {
      console.error('Build failed:', error)
    }
  }

  const filteredImages = images?.images.filter((image: ImageType) =>
    image.repository.toLowerCase().includes(searchTerm.toLowerCase()) ||
    image.tag.toLowerCase().includes(searchTerm.toLowerCase()) ||
    image.id.toLowerCase().includes(searchTerm.toLowerCase())
  ) || []

  const totalSize = images?.images.reduce((acc, image) => {
    const sizeStr = image.size.replace(/[^\d.]/g, '')
    const size = parseFloat(sizeStr) || 0
    return acc + size
  }, 0) || 0

  return (
    <div className="space-y-6">
      <div className="flex justify-between items-center">
        <div>
          <h1 className="text-3xl font-bold">Image Management</h1>
          <p className="text-gray-600 mt-2">Manage Docker images and registry operations</p>
        </div>
        <div className="flex space-x-2">
          <Button onClick={() => setShowPullDialog(true)}>
            <Plus className="h-4 w-4 mr-2" />
            Pull Image
          </Button>
          <Button onClick={() => refetch()} disabled={isLoading}>
            <RefreshCw className="h-4 w-4 mr-2" />
            Refresh
          </Button>
        </div>
      </div>

      {/* Stats */}
      <div className="grid gap-4 md:grid-cols-3">
        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Total Images</CardTitle>
            <HardDrive className="h-4 w-4 text-gray-500" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{images?.images.length || 0}</div>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Total Size</CardTitle>
            <HardDrive className="h-4 w-4 text-gray-500" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{totalSize.toFixed(1)} MB</div>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Operations</CardTitle>
          </CardHeader>
          <CardContent>
            <div className="flex flex-wrap gap-1">
              <Button size="sm" variant="outline" onClick={() => setShowTagDialog(true)}>
                <Tag className="h-3 w-3 mr-1" />
                Tag
              </Button>
              <Button size="sm" variant="outline" onClick={() => setShowLoadDialog(true)}>
                <Upload className="h-3 w-3 mr-1" />
                Load
              </Button>
              <Button size="sm" variant="outline" onClick={() => setShowSaveDialog(true)}>
                <Save className="h-3 w-3 mr-1" />
                Save
              </Button>
              <Button size="sm" variant="outline" onClick={() => setShowBuildDialog(true)}>
                <Hammer className="h-3 w-3 mr-1" />
                Build
              </Button>
            </div>
          </CardContent>
        </Card>
      </div>

      {/* Search */}
      <Card>
        <CardContent className="p-4">
          <div className="relative">
            <Search className="absolute left-3 top-3 h-4 w-4 text-gray-400" />
            <Input
              placeholder="Search images by repository, tag, or ID..."
              value={searchTerm}
              onChange={(e) => setSearchTerm(e.target.value)}
              className="pl-9"
            />
          </div>
        </CardContent>
      </Card>

      {/* Images Table */}
      <Card>
        <CardHeader>
          <CardTitle>Images</CardTitle>
          <CardDescription>
            {filteredImages.length} images found
          </CardDescription>
        </CardHeader>
        <CardContent>
          {filteredImages.length === 0 ? (
            <div className="text-center py-8">
              <HardDrive className="h-12 w-12 text-gray-400 mx-auto mb-4" />
              <p className="text-gray-500">No images found</p>
            </div>
          ) : (
            <Table>
              <TableHeader>
                <TableRow>
                  <TableHead>Repository</TableHead>
                  <TableHead>Tag</TableHead>
                  <TableHead>Size</TableHead>
                  <TableHead>Created</TableHead>
                  <TableHead>Actions</TableHead>
                </TableRow>
              </TableHeader>
              <TableBody>
                {filteredImages.map((image: ImageType) => (
                  <TableRow key={`${image.id}-${image.repository}-${image.tag}`}>
                    <TableCell>
                      <div>
                        <div className="font-medium max-w-xs" title={image.repository}>
                          {image.repository}
                        </div>
                        <div className="text-xs text-gray-500">
                          {image.id.substring(0, 20)}
                        </div>
                      </div>
                    </TableCell>
                    <TableCell>
                      <Badge variant="secondary">{image.tag}</Badge>
                    </TableCell>
                    <TableCell>{image.size}</TableCell>
                    <TableCell className="text-sm">{image.created}</TableCell>
                    <TableCell>
                      <div className="flex space-x-1">
                        <Button
                          size="sm"
                          variant="outline"
                          onClick={() => {
                            setTagSource(`${image.repository}:${image.tag}`)
                            setShowTagDialog(true)
                          }}
                        >
                          <Tag className="h-3 w-3" />
                        </Button>
                        
                        <Button
                          size="sm"
                          variant="outline"
                          onClick={() => {
                            setSaveImageName(`${image.repository}:${image.tag}`)
                            setShowSaveDialog(true)
                          }}
                        >
                          <Save className="h-3 w-3" />
                        </Button>
                        
                        <Button
                          size="sm"
                          variant="outline"
                          onClick={() => handleRemoveImage(image.id)}
                          disabled={removeImage.isPending}
                          className="text-red-600 hover:text-red-700"
                        >
                          <Trash2 className="h-3 w-3" />
                        </Button>
                      </div>
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          )}
        </CardContent>
      </Card>

      {/* Pull Image Dialog */}
      {showPullDialog && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
          <Card className="w-full max-w-md">
            <CardHeader>
              <CardTitle>Pull Image</CardTitle>
              <CardDescription>Pull an image from a Docker registry</CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              <Input
                placeholder="Image name (e.g., nginx:latest)"
                value={pullImageName}
                onChange={(e) => setPullImageName(e.target.value)}
              />
              <div className="flex space-x-2 justify-end">
                <Button variant="outline" onClick={() => setShowPullDialog(false)}>
                  Cancel
                </Button>
                <Button 
                  onClick={handlePullImage}
                  loading={pullImage.isPending}
                  disabled={!pullImageName.trim()}
                >
                  <Download className="h-4 w-4 mr-2" />
                  Pull
                </Button>
              </div>
            </CardContent>
          </Card>
        </div>
      )}

      {/* Tag Image Dialog */}
      {showTagDialog && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
          <Card className="w-full max-w-md">
            <CardHeader>
              <CardTitle>Tag Image</CardTitle>
              <CardDescription>Create a new tag for an existing image</CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              <Input
                placeholder="Source image (e.g., nginx:latest)"
                value={tagSource}
                onChange={(e) => setTagSource(e.target.value)}
              />
              <Input
                placeholder="Target tag (e.g., my-nginx:v1.0)"
                value={tagTarget}
                onChange={(e) => setTagTarget(e.target.value)}
              />
              <div className="flex space-x-2 justify-end">
                <Button variant="outline" onClick={() => setShowTagDialog(false)}>
                  Cancel
                </Button>
                <Button 
                  onClick={handleTagImage}
                  loading={tagImage.isPending}
                  disabled={!tagSource.trim() || !tagTarget.trim()}
                >
                  <Tag className="h-4 w-4 mr-2" />
                  Tag
                </Button>
              </div>
            </CardContent>
          </Card>
        </div>
      )}

      {/* Load Image Dialog */}
      {showLoadDialog && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
          <Card className="w-full max-w-md">
            <CardHeader>
              <CardTitle>Load Image</CardTitle>
              <CardDescription>Load an image from a tar file</CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              <div className="flex gap-2">
                <Input
                  placeholder="File path (e.g., /path/to/image.tar)"
                  value={loadFilePath}
                  onChange={(e) => setLoadFilePath(e.target.value)}
                  className="flex-1"
                />
                <Button
                  type="button"
                  variant="outline"
                  onClick={() => setShowLoadFilePicker(true)}
                >
                  Browse
                </Button>
                {loadFilePath && (
                  <Button
                    type="button"
                    variant="ghost"
                    size="sm"
                    onClick={() => setLoadFilePath('')}
                  >
                    <X className="h-4 w-4" />
                  </Button>
                )}
              </div>
              <div className="flex space-x-2 justify-end">
                <Button variant="outline" onClick={() => setShowLoadDialog(false)}>
                  Cancel
                </Button>
                <Button 
                  onClick={handleLoadImage}
                  loading={loadImage.isPending}
                  disabled={!loadFilePath.trim()}
                >
                  <Upload className="h-4 w-4 mr-2" />
                  Load
                </Button>
              </div>
            </CardContent>
          </Card>
        </div>
      )}

      {/* Save Image Dialog */}
      {showSaveDialog && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
          <Card className="w-full max-w-md">
            <CardHeader>
              <CardTitle>Save Image</CardTitle>
              <CardDescription>Save an image to a tar file</CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              <Input
                placeholder="Image name (e.g., nginx:latest)"
                value={saveImageName}
                onChange={(e) => setSaveImageName(e.target.value)}
              />
              <div className="flex gap-2">
                <Input
                  placeholder="Save path (e.g., /path/to/save/image.tar)"
                  value={saveFilePath}
                  onChange={(e) => setSaveFilePath(e.target.value)}
                  className="flex-1"
                />
                <Button
                  type="button"
                  variant="outline"
                  onClick={() => setShowSaveFilePicker(true)}
                >
                  Browse
                </Button>
                {saveFilePath && (
                  <Button
                    type="button"
                    variant="ghost"
                    size="sm"
                    onClick={() => setSaveFilePath('')}
                  >
                    <X className="h-4 w-4" />
                  </Button>
                )}
              </div>
              <div className="flex space-x-2 justify-end">
                <Button variant="outline" onClick={() => setShowSaveDialog(false)}>
                  Cancel
                </Button>
                <Button 
                  onClick={handleSaveImage}
                  loading={saveImage.isPending}
                  disabled={!saveImageName.trim() || !saveFilePath.trim()}
                >
                  <Save className="h-4 w-4 mr-2" />
                  Save
                </Button>
              </div>
            </CardContent>
          </Card>
        </div>
      )}

      {/* Build Image Dialog */}
      {showBuildDialog && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
          <Card className="w-full max-w-md">
            <CardHeader>
              <CardTitle>Build Image</CardTitle>
              <CardDescription>Build an image from a Dockerfile</CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              <div className="flex gap-2">
                <Input
                  placeholder="Dockerfile path (e.g., /path/to/Dockerfile)"
                  value={buildDockerfilePath}
                  onChange={(e) => setBuildDockerfilePath(e.target.value)}
                  className="flex-1"
                />
                <Button
                  type="button"
                  variant="outline"
                  onClick={() => setShowDockerfilePicker(true)}
                >
                  Browse
                </Button>
                {buildDockerfilePath && (
                  <Button
                    type="button"
                    variant="ghost"
                    size="sm"
                    onClick={() => setBuildDockerfilePath('')}
                  >
                    <X className="h-4 w-4" />
                  </Button>
                )}
              </div>
              <Input
                placeholder="Image name (e.g., my-app:latest)"
                value={buildImageName}
                onChange={(e) => setBuildImageName(e.target.value)}
              />
              <div className="flex gap-2">
                <Input
                  placeholder="Build context (e.g., /path/to/context)"
                  value={buildContext}
                  onChange={(e) => setBuildContext(e.target.value)}
                  className="flex-1"
                />
                <Button
                  type="button"
                  variant="outline"
                  onClick={() => setShowBuildContextPicker(true)}
                >
                  Browse
                </Button>
                {buildContext && (
                  <Button
                    type="button"
                    variant="ghost"
                    size="sm"
                    onClick={() => setBuildContext('')}
                  >
                    <X className="h-4 w-4" />
                  </Button>
                )}
              </div>
              <div className="flex space-x-2 justify-end">
                <Button variant="outline" onClick={() => setShowBuildDialog(false)}>
                  Cancel
                </Button>
                <Button 
                  onClick={handleBuildImage}
                  loading={buildImage.isPending}
                  disabled={!buildDockerfilePath.trim() || !buildImageName.trim() || !buildContext.trim()}
                >
                  <Hammer className="h-4 w-4 mr-2" />
                  Build
                </Button>
              </div>
            </CardContent>
          </Card>
        </div>
      )}

      {/* File Picker Dialogs */}
      {showLoadFilePicker && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-[60]">
          <div className="w-full max-w-4xl p-4">
            <MetaFilePicker
              title="Select Image Tar File"
              onPathSelect={(path) => {
                setLoadFilePath(path)
                setShowLoadFilePicker(false)
              }}
              onCancel={() => setShowLoadFilePicker(false)}
              allowFileSelection={true}
              allowDirectorySelection={false}
              placeholder="Browse and select a Docker image tar file..."
            />
          </div>
        </div>
      )}

      {showSaveFilePicker && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-[60]">
          <div className="w-full max-w-4xl p-4">
            <MetaFilePicker
              title="Select Save Location"
              onPathSelect={(path) => {
                setSaveFilePath(path)
                setShowSaveFilePicker(false)
              }}
              onCancel={() => setShowSaveFilePicker(false)}
              allowFileSelection={true}
              allowDirectorySelection={true}
              placeholder="Browse and select where to save the image tar file..."
            />
          </div>
        </div>
      )}

      {showDockerfilePicker && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-[60]">
          <div className="w-full max-w-4xl p-4">
            <MetaFilePicker
              title="Select Dockerfile"
              onPathSelect={(path) => {
                setBuildDockerfilePath(path)
                setShowDockerfilePicker(false)
              }}
              onCancel={() => setShowDockerfilePicker(false)}
              allowFileSelection={true}
              allowDirectorySelection={false}
              placeholder="Browse and select the Dockerfile..."
            />
          </div>
        </div>
      )}

      {showBuildContextPicker && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-[60]">
          <div className="w-full max-w-4xl p-4">
            <MetaFilePicker
              title="Select Build Context Directory"
              onPathSelect={(path) => {
                setBuildContext(path)
                setShowBuildContextPicker(false)
              }}
              onCancel={() => setShowBuildContextPicker(false)}
              allowFileSelection={false}
              allowDirectorySelection={true}
              placeholder="Browse and select the build context directory..."
            />
          </div>
        </div>
      )}
    </div>
  )
}

export default ImageManagement
