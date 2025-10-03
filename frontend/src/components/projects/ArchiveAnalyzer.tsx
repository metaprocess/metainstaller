import { useState } from 'react'
import { useMutation } from '@tanstack/react-query'
import { X, FileArchive, Lock, CheckCircle, XCircle, File } from 'lucide-react'
import { DockerApiService } from '@/services/api'
import { Button, Card, CardHeader, CardTitle, CardContent, Input, MetaFilePicker, Badge, Alert, AlertDescription } from '@/components/ui'
import type { ProjectArchiveInfo } from '@/types/api'

interface ArchiveAnalyzerProps {
  onClose: () => void
}

export const ArchiveAnalyzer = ({ onClose }: ArchiveAnalyzerProps) => {
  const [archivePath, setArchivePath] = useState('')
  const [password, setPassword] = useState('')
  const [analysisResult, setAnalysisResult] = useState<ProjectArchiveInfo | null>(null)
  const [showFilePicker, setShowFilePicker] = useState(false)

  const analyzeArchiveMutation = useMutation({
    mutationFn: (data: { archive_path: string; password?: string }) => 
      DockerApiService.analyzeArchive(data),
    onSuccess: (result) => {
      setAnalysisResult(result)
    },
  })

  const handlePathSelect = (path: string) => {
    setArchivePath(path)
    setShowFilePicker(false)
  }

  const handleAnalyze = () => {
    if (!archivePath) return
    
    analyzeArchiveMutation.mutate({
      archive_path: archivePath,
      password: password || undefined
    })
  }

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-white rounded-lg shadow-xl max-w-2xl w-full max-h-[90vh] overflow-auto mx-4">
        <div className="flex items-center justify-between p-6 border-b">
          <h2 className="text-xl font-semibold">Analyze Archive</h2>
          <Button variant="ghost" size="sm" onClick={onClose}>
            <X className="h-4 w-4" />
          </Button>
        </div>

        <div className="p-6 space-y-6">
          <div className="space-y-4">
            <div>
              <label className="block text-sm font-medium mb-2">Archive File</label>
              <div className="flex gap-2">
                <Input
                  value={archivePath}
                  onChange={(e) => setArchivePath(e.target.value)}
                  placeholder="Select archive file (.7z, .zip, .tar, .tar.gz)"
                  className="flex-1"
                />
                <Button
                  type="button"
                  variant="outline"
                  onClick={() => setShowFilePicker(true)}
                >
                  Browse
                </Button>
                {archivePath && (
                  <Button
                    type="button"
                    variant="ghost"
                    size="sm"
                    onClick={() => setArchivePath('')}
                  >
                    <X className="h-4 w-4" />
                  </Button>
                )}
              </div>
            </div>

            <div>
              <label className="block text-sm font-medium mb-2">Password (optional)</label>
              <Input
                type="password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                placeholder="Enter password if archive is encrypted"
              />
            </div>

            <Button
              onClick={handleAnalyze}
              disabled={!archivePath || analyzeArchiveMutation.isPending}
              loading={analyzeArchiveMutation.isPending}
              className="w-full"
            >
              <FileArchive className="h-4 w-4 mr-2" />
              Analyze Archive
            </Button>
          </div>

          {analyzeArchiveMutation.error && (
            <Alert>
              <XCircle className="h-4 w-4" />
              <AlertDescription>
                Failed to analyze archive: {analyzeArchiveMutation.error.message}
              </AlertDescription>
            </Alert>
          )}

          {analysisResult && (
            <Card>
              <CardHeader>
                <CardTitle className="flex items-center gap-2">
                  Analysis Results
                  {analysisResult.success ? (
                    <CheckCircle className="h-5 w-5 text-green-600" />
                  ) : (
                    <XCircle className="h-5 w-5 text-red-600" />
                  )}
                </CardTitle>
              </CardHeader>
              <CardContent className="space-y-4">
                {!analysisResult.success && (
                  <Alert>
                    <XCircle className="h-4 w-4" />
                    <AlertDescription>
                      {analysisResult.error_message}
                    </AlertDescription>
                  </Alert>
                )}

                {analysisResult.success && (
                  <>
                    <div className="grid grid-cols-2 gap-4">
                      <div>
                        <span className="text-sm font-medium text-gray-500">Encrypted</span>
                        <div className="flex items-center gap-2">
                          {analysisResult.is_encrypted ? (
                            <>
                              <Lock className="h-4 w-4 text-amber-600" />
                              <Badge className="bg-amber-100 text-amber-800">Yes</Badge>
                            </>
                          ) : (
                            <>
                              <CheckCircle className="h-4 w-4 text-green-600" />
                              <Badge className="bg-green-100 text-green-800">No</Badge>
                            </>
                          )}
                        </div>
                      </div>
                      
                      <div>
                        <span className="text-sm font-medium text-gray-500">Integrity</span>
                        <div className="flex items-center gap-2">
                          {analysisResult.integrity_verified ? (
                            <>
                              <CheckCircle className="h-4 w-4 text-green-600" />
                              <Badge className="bg-green-100 text-green-800">Verified</Badge>
                            </>
                          ) : (
                            <>
                              <XCircle className="h-4 w-4 text-red-600" />
                              <Badge className="bg-red-100 text-red-800">Failed</Badge>
                            </>
                          )}
                        </div>
                      </div>
                    </div>

                    <div>
                      <span className="text-sm font-medium text-gray-500 mb-2 block">
                        Docker Images ({analysisResult.docker_images.length})
                      </span>
                      <div className="space-y-1 max-h-32 overflow-y-auto">
                        {analysisResult.docker_images.map((image, index) => (
                          <div key={index} className="flex items-center gap-2 text-sm">
                            <File className="h-3 w-3 text-gray-400" />
                            <code className="bg-gray-100 px-2 py-1 rounded text-xs">{image}</code>
                          </div>
                        ))}
                      </div>
                    </div>

                    <div>
                      <span className="text-sm font-medium text-gray-500 mb-2 block">
                        Contained Files ({analysisResult.contained_files.length})
                      </span>
                      <div className="space-y-1 max-h-40 overflow-y-auto">
                        {analysisResult.contained_files.slice(0, 20).map((file, index) => (
                          <div key={index} className="flex items-center gap-2 text-sm">
                            <File className="h-3 w-3 text-gray-400" />
                            <span className="font-mono text-xs">{file}</span>
                          </div>
                        ))}
                        {analysisResult.contained_files.length > 20 && (
                          <p className="text-xs text-gray-500 italic">
                            ...and {analysisResult.contained_files.length - 20} more files
                          </p>
                        )}
                      </div>
                    </div>
                  </>
                )}
              </CardContent>
            </Card>
          )}
        </div>

        <div className="flex justify-end gap-2 p-6 border-t">
          <Button variant="outline" onClick={onClose}>
            Close
          </Button>
        </div>
      </div>

      {showFilePicker && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-[60]">
          <div className="w-full max-w-4xl p-4">
            <MetaFilePicker
              title="Select Archive File"
              onPathSelect={handlePathSelect}
              onCancel={() => setShowFilePicker(false)}
              allowFileSelection={true}
              allowDirectorySelection={false}
              placeholder="Browse and select an archive file (.7z, .zip, .tar, .tar.gz)..."
            />
          </div>
        </div>
      )}
    </div>
  )
}