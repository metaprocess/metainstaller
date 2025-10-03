import { useState } from 'react'
import {
  useDockerInfo,
  useDockerComposeInfo,
  useInstallDocker,
  useUninstallDocker,
  useInstallationProgress,
  useTestSudo
} from '@/hooks/useApi'
import { Card, CardContent, CardDescription, CardHeader, CardTitle, Button, Input, Alert, AlertDescription } from '@/components/ui'
import { Badge } from '@/components/ui'
import { Download, Trash2, CheckCircle, XCircle, AlertCircle, Lock } from 'lucide-react'

const Installation = () => {
  const [sudoPassword, setSudoPassword] = useState('')
  const [showPasswordDialog, setShowPasswordDialog] = useState(false)
  const [pendingAction, setPendingAction] = useState<'install' | 'uninstall' | null>(null)

  const { data: dockerInfo, refetch: refetchDockerInfo } = useDockerInfo()
  const { data: composeInfo, refetch: refetchComposeInfo } = useDockerComposeInfo()
  const installMutation = useInstallDocker()
  const uninstallMutation = useUninstallDocker()
  const testSudoMutation = useTestSudo()
  const { data: progress } = useInstallationProgress(installMutation.isPending || uninstallMutation.isPending)

  const handleSudoSubmit = async () => {
    try {
      await testSudoMutation.mutateAsync({ password: sudoPassword })
      setShowPasswordDialog(false)

      if (pendingAction === 'install') {
        installMutation.mutate(undefined, {
          onSuccess: () => {
            refetchDockerInfo()
            refetchComposeInfo()
          }
        })
      } else if (pendingAction === 'uninstall') {
        uninstallMutation.mutate(undefined, {
          onSuccess: () => {
            refetchDockerInfo()
            refetchComposeInfo()
          }
        })
      }

      setPendingAction(null)
      setSudoPassword('')
    } catch (error) {
      console.error('Sudo authentication failed:', error)
    }
  }

  const handleInstall = () => {
    setPendingAction('install')
    setShowPasswordDialog(true)
  }

  const handleUninstall = () => {
    setPendingAction('uninstall')
    setShowPasswordDialog(true)
  }

  const getStatusIcon = (installed: boolean) => {
    return installed ? (
      <CheckCircle className="h-5 w-5 text-green-500" />
    ) : (
      <XCircle className="h-5 w-5 text-red-500" />
    )
  }

  const getStatusBadge = (installed: boolean) => {
    return (
      <Badge variant={installed ? 'success' : 'destructive'}>
        {installed ? 'Installed' : 'Not Installed'}
      </Badge>
    )
  }

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold">Main Installation</h1>
        <p className="text-gray-600 mt-2">Install or uninstall Main Service</p>
      </div>

      {progress?.installation_in_progress && (
        <Alert>
          <AlertDescription>
            <div className="flex items-center space-x-2">
              <AlertCircle className="h-4 w-4" />
              <span>Installation in progress: {progress.message} ({progress.percentage}%)</span>
            </div>
          </AlertDescription>
        </Alert>
      )}

      {progress?.error_details && (
        <Alert variant="destructive">
          <AlertDescription>
            <div className="flex items-center space-x-2">
              <XCircle className="h-4 w-4" />
              <span>Error: {progress.error_details}</span>
            </div>
          </AlertDescription>
        </Alert>
      )}

      <div className="grid gap-6 md:grid-cols-2">
        <Card>
          <CardHeader>
            <div className="flex items-center justify-between">
              <div>
                <CardTitle className="flex items-center space-x-2">
                  {getStatusIcon(dockerInfo?.installed || false)}
                  <span>Docker Engine</span>
                </CardTitle>
                <CardDescription>Container runtime and management</CardDescription>
              </div>
              {getStatusBadge(dockerInfo?.installed || false)}
            </div>
          </CardHeader>
          <CardContent className="space-y-4">
            {dockerInfo?.installed && dockerInfo.version && (
              <div className="space-y-2 text-sm">
                <div className="flex justify-between">
                  <span className="text-gray-500">Version:</span>
                  <span>{dockerInfo.version}</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-gray-500">API Version:</span>
                  <span>{dockerInfo.api_version}</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-gray-500">Architecture:</span>
                  <span>{dockerInfo.arch}</span>
                </div>
                <div className="flex justify-between">
                  <span className="text-gray-500">OS:</span>
                  <span>{dockerInfo.os}</span>
                </div>
              </div>
            )}

            {dockerInfo?.error_message && (
              <Alert variant="destructive">
                <AlertDescription>{dockerInfo.error_message}</AlertDescription>
              </Alert>
            )}

            <div className="flex space-x-2">
              {!dockerInfo?.installed ? (
                <Button
                  onClick={handleInstall}
                  loading={installMutation.isPending}
                  disabled={progress?.installation_in_progress}
                >
                  <Download className="h-4 w-4 mr-2" />
                  Install Docker
                </Button>
              ) : (
                <Button
                  variant="destructive"
                  onClick={handleUninstall}
                  loading={uninstallMutation.isPending}
                  disabled={progress?.installation_in_progress}
                >
                  <Trash2 className="h-4 w-4 mr-2" />
                  Uninstall Docker
                </Button>
              )}
            </div>
          </CardContent>
        </Card>

        <Card>
          <CardHeader>
            <div className="flex items-center justify-between">
              <div>
                <CardTitle className="flex items-center space-x-2">
                  {getStatusIcon(composeInfo?.installed || false)}
                  <span>Docker Compose</span>
                </CardTitle>
                <CardDescription>Multi-container application management</CardDescription>
              </div>
              {getStatusBadge(composeInfo?.installed || false)}
            </div>
          </CardHeader>
          <CardContent className="space-y-4">
            {composeInfo?.installed && composeInfo.version && (
              <div className="space-y-2 text-sm">
                <div className="flex justify-between">
                  <span className="text-gray-500">Version:</span>
                  <span>{composeInfo.version}</span>
                </div>
              </div>
            )}

            {composeInfo?.error_message && (
              <Alert variant="destructive">
                <AlertDescription>{composeInfo.error_message}</AlertDescription>
              </Alert>
            )}

            <div className="text-sm text-gray-600">
              Docker Compose is typically installed with Docker Engine and managed automatically.
            </div>
          </CardContent>
        </Card>
      </div>

      {/* Sudo Password Dialog */}
      {showPasswordDialog && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
          <Card className="w-full max-w-md">
            <CardHeader>
              <CardTitle className="flex items-center space-x-2">
                <Lock className="h-5 w-5" />
                <span>Sudo Authentication Required</span>
              </CardTitle>
              <CardDescription>
                Enter your sudo password to {pendingAction} Docker
              </CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              <Input
                type="password"
                placeholder="Sudo password"
                value={sudoPassword}
                onChange={(e) => setSudoPassword(e.target.value)}
                onKeyDown={(e) => e.key === 'Enter' && handleSudoSubmit()}
              />

              {!testSudoMutation.error ? null : (
                <Alert variant="destructive">
                  <AlertDescription>
                    Authentication failed. Please check your password.
                  </AlertDescription>
                </Alert>
              )}

              <div className="flex space-x-2 justify-end">
                <Button
                  variant="outline"
                  onClick={() => {
                    setShowPasswordDialog(false)
                    setPendingAction(null)
                    setSudoPassword('')
                  }}
                >
                  Cancel
                </Button>
                <Button
                  onClick={handleSudoSubmit}
                  loading={testSudoMutation.isPending}
                  disabled={!sudoPassword}
                >
                  Authenticate
                </Button>
              </div>
            </CardContent>
          </Card>
        </div>
      )}
    </div>
  )
}

export default Installation